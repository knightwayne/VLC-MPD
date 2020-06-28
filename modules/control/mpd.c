#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
/* VLC core API headers */
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
/*Added during MPD Development*/
#include <vlc_fs.h>
#include <vlc_network.h>


/*Port and Address to look up at Request at - http://192.168.0.101:6600*/
#define PORT 6600

/*<---------------------------Core MPD Server----------------------------->*/

//System Variables Declaration Here.
struct intf_sys_t
{
    //xxxxx
    int serverFD;
    vlc_thread_t th;
};

void *thread_func(void *arg) //Parsing Polling Listening to Requests
{
    intf_thread_t *intfa = (intf_thread_t *)arg;
    msg_Info(intfa, "Hello from thread side!");
    int server_fd=intfa->p_sys->serverFD;

    //thread work;
    int client_fd;
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    // Listening to clients
    if (listen(server_fd, SOMAXCONN) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //Select case handling-------------------------------------------------------

    // New Connection Comes, Handling It
    if ((client_fd = accept(server_fd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientAddrLen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }



    //Select case handling---

    //Parsing
    char *vlc = "dummy"; //dummy valid command like play pause

    while (1) //is this (infinite while loop) the correct way to keep listensing requests in the background??
    {
        int n;
        char buffer[256];

        //Recieve
        bzero(buffer, 256);
        n = read(client_fd, buffer, 255); //client_fd to be passed as argument in intf;
        if (n < 0)
            printf("ERROR reading from socket");
        else
        {
            printf("%d Buffer %s %c\n", strlen(buffer), buffer, buffer[2]);
        }

        //Action & Send
        if (!strcmp(buffer, vlc)) //one of the valid commands--MAIN ACTION AND PLAYBACK CONTROL HERE
        {
            //send(client_fd, ok, strlen(ok), 0);
            printf("OK vlc opening\n");
            system(buffer);
        }
        else //not one of the valid commands
        {
            //send(client_fd, nok, strlen(nok), 0);
            printf("Not OK Terminating\n");
            break;
        }
    }

    return NULL;
}

/*Open Module*/
static int Open(vlc_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj; //thread for interface debugging
    msg_Info(intf, "Hello World! MPD Server Started");

    // Server Creation
    // Initialising Server
    int server_fd;
    struct sockaddr_in serverAddr;
    int opt = 1;
    //int serverAddrLen = sizeof(serverAddr);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Error Opening Socket. Socket Failed");
        exit(EXIT_FAILURE);
    }
    bzero((char *)&serverAddr, sizeof(serverAddr));

    // optional
    // Forcefully attaching socket to the port 6600
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR /*SO_REUSEPORT*/, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 6600(binding)
    if (bind(server_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Thread Creation to listen to requests
    intf_thread_t *intfa = (intf_thread_t *)obj;    //thread for passing objects;
    vlc_thread_t mpd_t;
    
    intf_sys_t *sys = malloc(sizeof(*sys));
    if (unlikely(sys == NULL))
        return VLC_ENOMEM;
    intfa->p_sys=sys;
    sys->serverFD=server_fd;
    sys->th=mpd_t;

    int i = vlc_clone(&sys->th, thread_func, intfa, 0);
    msg_Info(intfa, "Thread Started from Open() %d!", i);

    return VLC_SUCCESS;
    //error case -> add go to statements if needed
}

/*Close Module*/
static void Close(vlc_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj;
    intf_sys_t *sys = intf->p_sys;
    
    vlc_join(sys->th, NULL);
    msg_Info(intf, "Good bye MPD Server");
    
    //free(&sys->th);
    free(sys->serverFD);
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
