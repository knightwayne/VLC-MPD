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

/*Port and Address to look up at Request at - http://192.168.0.101:6600 .. 4_yhoyhJegDyAGxR3fSu*/
#define PORT 6600

// Function Declarations & System Variables Declaration Here.
void *clientHandling(void *arg);
void *parseRequest(int client_fd, fd_set* currentSockets);

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

/*client request parsing function*/
void* parseRequest(int client_fd, fd_set *currentSockets)
{
    int n;
    char buffer[1024];
    char buffer1[1024];
    bzero(buffer, 1024);

    if ((n = read(client_fd, buffer1, 1023)) < 0)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }

    //printf("BufferSize: %ld \tBuffer %s \n", strlen(buffer1), buffer1);
    
    if(strlen(buffer1)!=0)
    memcpy(buffer,buffer1,strlen(buffer)-1);

    if(strlen(buffer)==0)
    {
        printf("01BufferSize: %ld \tBuffer %s \n", strlen(buffer), buffer);
        send(client_fd, "ACK", strlen("ACK"), 0); //add command list
        FD_CLR(client_fd, currentSockets);
    }
    else if (!strcmp(buffer, "commands"))
    {
        printf("02BufferSize: %ld \tBuffer %s \n", strlen(buffer), buffer);
        send(client_fd, "command list\nplay\npause", strlen("command list\nplay\npause"), 0); //add command list & send back
    }
    else if (!strcmp(buffer, "play"))
    {
        printf("03BufferSize: %ld \tBuffer %s \n", strlen(buffer), buffer);
        send(client_fd, "play", strlen("play"), 0); //play
    }
    else if (!strcmp(buffer, "pause"))
    {
        printf("04BufferSize: %ld \tBuffer %s \n", strlen(buffer), buffer);
        send(client_fd, "pause", strlen("pause"), 0); //pause
    }
    else//if no command is found.
    {
        printf("05BufferSize: %ld \tBuffer %s \n", strlen(buffer), buffer);
        FD_CLR(client_fd, currentSockets); //when quit is called
    }

    return NULL;
}

/*client Handling thread*/
void* clientHandling(void *arg) //Parsing Polling Listening to Requests
{
    intf_thread_t *intfa = (intf_thread_t *)arg;
    msg_Info(intfa, "Hello from thread side!");

    int server_fd = intfa->p_sys->server_fd;
    int client_fd;
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);


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


    // 9. EVENT LOOP ->to listen to changes
    while(true)
    {
        readySockets=currentSockets;
        if(select(FD_SETSIZE, &readySockets,NULL,NULL,NULL)<0)  //server has read operation pending
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if(FD_ISSET(i,&readySockets))
            {
                if(i==server_fd) //server accept condition
                {
                    if ((client_fd = accept(server_fd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientAddrLen)) < 0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(client_fd,&currentSockets);
                }
                else //client pending read connection
                {
                    client_fd = i;
                    msg_Info(intfa, "client_fd read-> %d\n", i);
                    parseRequest(client_fd,&currentSockets);
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
