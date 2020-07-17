#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

/* VLC core API headers */
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>

/* Added during MPD Server Development */
#include <sys/select.h>
#include <vlc_fs.h>
#include <vlc_network.h>
#include <vlc_vector.h>

/* Added during Playback Development */
#include <vlc_media_library.h>
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_input_item.h>
#include <vlc_aout.h>
#include <vlc_vout.h>
#include <vlc_player.h>
#include <vlc_playlist.h>
#include <vlc_actions.h>

/* Port and Address to look up at Request at - http://192.168.0.101:6600 .. 4_yhoyhJegDyAGxR3fSu */
#define PORT 6600

/*struct and array declarations*/
typedef struct commandFunc
{
    const char *command;
    char* (*commandName)();/*some-variable for passing*/
}commandFunc;

/*Function Declarations*/
void* clientHandling(void *arg);
static int string_compare_function(const void *key, const void *arrayElement);
commandFunc* searchCommand(const char* key);
char* mediaList(void);
char* mediaList2(void);
char* quit(void);

/*command List Array*/
const commandFunc command_list_array[]={
    {"buffer", mediaList},
    {"noidle", mediaList2},
    {"quit", quit}
};

/*System Variables Declaration*/
struct intf_sys_t
{
    int server_fd;
    vlc_thread_t mpd_thread;
};

/*command functions - main media playback here*/
char* mediaList(void)
{
    printf("mediaList\n");
    //vlc_medialibrary_t* vlcmt;
    return "mediaList";
}
char* mediaList2(void)
{
    printf("mediaList2\n");



    return "mediaList2";
}
char* quit(void)
{
    printf("quit\n");
    return "quit";
}

/*string_compare_function*/
static int string_compare_function(const void *key, const void *arrayElement)
{
    const commandFunc *commandFunc = arrayElement;
    return strcmp(key, commandFunc->command);/*each list item's string binary search*/
}

/*command search function*/
commandFunc* searchCommand(const char* key)
{
    return bsearch(key, command_list_array, ARRAY_SIZE(command_list_array),sizeof(*command_list_array), string_compare_function);
}

/*client Handling thread*/
void* clientHandling(void *arg) //Parsing Polling Listening to Requests
{
    // 0. Function Start
    intf_thread_t *intfa = (intf_thread_t *)arg;
    msg_Info(intfa, "Hello from thread side!");

    // 1. Initialising Client
    int server_fd = intfa->p_sys->server_fd;
    struct sockaddr_in clientAddr;
    int client_fd,clientAddrLen = sizeof(clientAddr);

    fd_set currentSockets,readySockets;
    FD_ZERO(&currentSockets);
    FD_SET(server_fd,&currentSockets);  //adding server_fd to currentSockets

    // 2. Listening to clients
    if (listen(server_fd, SOMAXCONN) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 3. EVENT LOOP -> to listen to changes continuously from clients
    while(true)
    {
        readySockets=currentSockets;
        // 4. Accepting(Monitoring) Multiple CLients using Select() method for polling
        if(select(FD_SETSIZE, &readySockets,NULL,NULL,NULL)<0)  //readySockets has read operation pending
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if(FD_ISSET(i,&readySockets))
            {
                // server has new client connection
                if(i==server_fd) 
                {
                    bzero((char *)&clientAddr, sizeof(clientAddr));
                    if ((client_fd = accept(server_fd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientAddrLen)) < 0)
                    {
                        perror("accept");   
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(client_fd,&currentSockets);  //move client_fd to current
                    msg_Info(intfa, "server read: %d\n", client_fd);
                }
                // client has pending read connection (new message)
                else 
                {
                    client_fd = i;
                    msg_Info(intfa, "client_fd read: %d\n", client_fd);
                    
                    int n;
                    char input[4096];
                    bzero(input,4096);
                    if ((n = read(client_fd,input,4095)) < 0)
                    {
                        perror("read");
                        exit(EXIT_FAILURE);
                    }
                    input[strlen(input)-1]='\0';
                    //if length is 0-> send OK, study MPD protocol for this;
                    printf("input:%s\t inputSize:%ld\n",input,strlen(input));

                    commandFunc* command=searchCommand(input);
                    if(command==NULL)
                    {
                        printf("command not found\n");
                        send(client_fd, "NOK", strlen("NOK"), 0);
                
                        // call media listing and playback functions
                        //<--here-->
                        FD_CLR(client_fd, &currentSockets); //remove client_fd to current
                        continue;
                    }

                    char* output = command->commandName(); 
                    send(client_fd, output, strlen(output), 0);

                    //should close connection after serving request or not???
                    FD_CLR(client_fd, &currentSockets); //remove client_fd to current
                }
            }//if
        }//for
    }//while

    // 5. Function End
    return NULL;
}

/*Open Module*/
static int Open(vlc_object_t *obj)
{
    // 0. Module Start 
    intf_thread_t *intf = (intf_thread_t *)obj; //thread for interface debugging
    msg_Info(intf, "Hello World! MPD Server Started");

    // 1. Initialising Server
    int server_fd;
    struct sockaddr_in serverAddr;

    // 2. Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Error Opening Socket. Socket Failed");
        exit(EXIT_FAILURE);
    }
    bzero((char *)&serverAddr, sizeof(serverAddr));

    // 3. Forcefully attaching socket to the port 6600 - OPTIONAL
    int opt=1;
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

    // 5. Thread Creation & Initialisation
    intf_thread_t *intfa = (intf_thread_t *)obj;    //thread for passing objects;
    vlc_thread_t mpd_t;
    intf_sys_t *sys = malloc(sizeof(*sys));
    if (unlikely(sys == NULL))
        return VLC_ENOMEM;
    
    intfa->p_sys=sys;
    sys->mpd_thread = mpd_t;
    sys->server_fd=server_fd;

    // 6. Client Handling
    int i=0;
    if((i = vlc_clone(&sys->mpd_thread, clientHandling, intfa, 0))<0) //0 - VLC_THREAD_PRIORITY_LOW
    {
        perror("clone failed");
        exit(EXIT_FAILURE);
    }

    // 7. Module End
    return VLC_SUCCESS;
    //error case -> add go to statements if needed
}

/*Close Module*/
static void Close(vlc_object_t *obj)
{
    // 0. Module Start
    intf_thread_t *intf = (intf_thread_t *)obj;
    
    // 1. Freeing up resources
    // - those dynamically allocated, not static allocation (like int i = 10;)
    // check for appropriate dynamic allocation, I have used only static allocation
    intf_sys_t *sys = intf->p_sys;
    vlc_join(sys->mpd_thread, NULL);
    free(sys);

    // 2. Module End
    msg_Info(intf, "Good bye MPD Server");
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
