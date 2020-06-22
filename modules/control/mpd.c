#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
/* VLC core API headers */
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_fs.h>
#include <vlc_network.h>
// #include <vlc_objects.h>
// #include <vlc_variables.h>
#define PORT 6600
// Request at - http://192.168.0.101:6600/

struct intf_sys_t
{
    //define variables needed for the module
};

void *thread_func(void *arg)
{
    //thread has a charString variable to be added for keep listening to requests until stopped, and closed in close(); 

    intf_thread_t *intfa = (intf_thread_t *)arg;
    // char *a = vlc_getcwd();
    // msg_Info(intfa, "%s!", a);

    //Parsing
    char *vlc ="dummy"; //dummy valid command like play pause

    while (1)           //is this (infinite while loop) the correct way to keep listensing requests in the background??
    {
        int n;
        char buffer[256];

        //Recieve
        bzero(buffer, 256);
        n = read(new_socket, buffer, 255);  //new_socket to be passed as argument in intf;
        if (n < 0)
            printf("ERROR reading from socket");
        else
        {
            printf("%d Buffer %s %c\n", strlen(buffer), buffer, buffer[2]);
        }

        //Action & Send
        if (!strcmp(buffer, vlc))   //one of the valid commands--MAIN ACTION AND PLAYBACK CONTROL HERE
        {
            //send(new_socket, ok, strlen(ok), 0);
            printf("OK vlc opening\n");
            system(buffer);
        }
        else                        //not one of the valid commands
        {
            //send(new_socket, nok, strlen(nok), 0);
            printf("Not OK Terminating\n");
            break;
        }
    }

    return NULL;
}

static int Open(vlc_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj;
    msg_Info(intf, "Hello World! MPD Server Started");

    //SERVER CREATION--------------------------------------------
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR /*| SO_REUSEPORT*/, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    //CREATING THREAD TO LISTEN TO REQUESTS--------------------------------------------------
    intf_thread_t *intfa = (intf_thread_t *)obj;
    vlc_thread_t th;
    int i = vlc_clone(&th, thread_func, intfa, 0);
    msg_Info(intfa, "%d!", i);
    vlc_join(th, NULL);

    return VLC_SUCCESS;
}

static void Close(vlc_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj;
    msg_Info(intf, "Good bye MPD Server");
}

/* Module descriptor */
vlc_module_begin()
    set_shortname(N_("MPD"))
        set_description(N_("MPD Control Interface Module"))
            set_capability("interface", 0)
                set_callbacks(Open, Close)
                    set_category(CAT_INTERFACE)
    //add_string("variable", "world", "Target", "Whom to say hello to.", false) //?
    vlc_module_end()

