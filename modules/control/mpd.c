#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

/* VLC core API headers */
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>

/*Added during MPD Server Development*/
#include <sys/select.h>
#include <vlc_fs.h>
#include <vlc_network.h>
#include <vlc_vector.h>

/*Added during Playback Development*/
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_input_item.h>
#include <vlc_aout.h>
#include <vlc_vout.h>
#include <vlc_player.h>
#include <vlc_playlist.h>
#include <vlc_actions.h>

/*Port and Address to look up at Request at - http://192.168.0.101:6600*/
#define PORT 6600

// Function Declarations & System Variables Declaration Here.
void *clientHandling(void *arg);
void *parseRequest(int client_fd);

struct intf_sys_t
{
    int server_fd;
    vlc_thread_t mpd_thread;
    struct clientInfo
    {
        int client_fd;
        char *buffer[1024];
    };
    struct VLC_VECTOR(struct clientInfo) vec;
};

// typedef struct VLC_VECTOR(int) cl_vec;

/*---------------------------------------MPD Server Code-------------------------------------------*/
/*---------------------------------------MPD Server Code-------------------------------------------*/
/*---------------------------------------MPD Server Code-------------------------------------------*/

void* parseRequest(int client_fd)
{
    int n;
    const char *vlc = "command";
    const char *ok = "ack", *nok = "list";
    char buffer[1024];
    //Recieve
    bzero(buffer, 1024);

    if ((n = read(client_fd, buffer, 1023)) < 0) //client_fd to be passed as argument in intf;
    {
        perror("read");
        exit(EXIT_FAILURE);
    }

    printf("BufferSize: %ld \tBuffer %s \n", strlen(buffer), buffer);
    //msg_Info(intfa, "BufferSize: %ld \tBuffer %s \n", strlen(buffer), buffer);

    // 9. Action & Send
    //Main Parsing Function
    send(client_fd, nok, strlen(nok), 0);
}

void* clientHandling(void *arg) //Parsing Polling Listening to Requests
{
    intf_thread_t *intfa = (intf_thread_t *)arg;
    msg_Info(intfa, "Hello from thread side!");

    int server_fd = intfa->p_sys->server_fd;
    int client_fd;
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    //struct VLC_VECTOR(int) vec = VLC_VECTOR_INITIALIZER;


    // 7. Listening to clients
    if (listen(server_fd, SOMAXCONN) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }


    // 8. Accepting Multiple CLients using Select() method for polling
    fd_set currentSockets,readySockets;
    FD_ZERO(&currentSockets);
    FD_SET(server_fd,&currentSockets);  //adding server_fd to currentSockets


    // 9. EVENT LOOP
    while(true)
    {
        readySockets=currentSockets;
        if(select(FD_SETSIZE, &readySockets,NULL,NULL,NULL)<0)  //server has read operation pending
        {
            perror("select");
            exit(EXIT_FAILURE);
        }
        //msg_Info(intfa, "Enter for loop\n");

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if(FD_ISSET(i,&readySockets))
            {
                if(i==server_fd) //read instructions pending at server - ie accept pending at server
                {
                    // New Connection Comes, Handling It
                    if ((client_fd = accept(server_fd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientAddrLen)) < 0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(client_fd,&currentSockets);  //next time if read instructions are pending at client_fd, so to check them
                    //vlc_vector_push(&vec,client_fd);    //pushing in client_fd, to maintain vector of clients 
                }
                else //read pending at client_fd
                {
                    // Handle Client Request 
                    client_fd = i;
                    msg_Info(intfa, "client_fd %d\n", i);
                    parseRequest(client_fd);
                    //FD_CLR(i,&currentSockets);    //after client_fd is handled, we can free it
                }
            }//if
        }//for
    }//while

    return NULL;
}

/*Open Module*/
static int Open(vlc_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj; //thread for interface debugging
    msg_Info(intf, "Hello World! MPD Server Started");

    // Server Creation
    // 1. Initialising Server
    int server_fd;
    int opt = 1;
    struct sockaddr_in serverAddr;


    // 2. Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Error Opening Socket. Socket Failed");
        exit(EXIT_FAILURE);
    }
    bzero((char *)&serverAddr, sizeof(serverAddr));


    // 3. Forcefully attaching socket to the port 6600 - OPTIONAL
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);


    // 4. Forcefully attaching socket to the port 6600(binding)
    if (bind(server_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }


    // 5. Thread Creation to listen to client requests
    intf_thread_t *intfa = (intf_thread_t *)obj;    //thread for passing objects;
    vlc_thread_t mpd_t;     //warning - handle uninitialised - FIX IT
    
    intf_sys_t *sys = malloc(sizeof(*sys));
    if (unlikely(sys == NULL))
        return VLC_ENOMEM;

    intfa->p_sys=sys;
    sys->mpd_thread = mpd_t;
    sys->server_fd=server_fd;

    int i = vlc_clone(&sys->mpd_thread, clientHandling, intfa, 0); //0 - VLC_THREAD_PRIORITY_LOW

    return VLC_SUCCESS;
    //error case -> add go to statements if needed
}

/*Close Module*/
static void Close(vlc_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj;
    intf_sys_t *sys = intf->p_sys;
    
    vlc_join(sys->mpd_thread, NULL);
    msg_Info(intf, "Good bye MPD Server");

    free(sys);
}

/* Module descriptor */
vlc_module_begin()
    set_shortname(N_("MPD"))
    set_description(N_("MPD Control Interface Module"))
    set_capability("interface", 0)
    set_callbacks(Open, Close)
    set_category(CAT_INTERFACE)
    //add_string("variable", "world", "Target", "Whom to say hello to.", false) //--use case not known til now!
vlc_module_end()
