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
#include <vlc_memstream.h>

/* Added during Playback Development */
#include <vlc_media_library.h>
#include <vlc_player.h>
#include <vlc_playlist.h>
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_input_item.h>
#include <vlc_aout.h>
#include <vlc_vout.h>
#include <vlc_actions.h>

/* Port and Address to look up at Request at - http://192.168.0.101:6600 .. 4_yhoyhJegDyAGxR3fSu */
//sudo gedit /etc/mpd.conf 
#define PORT 6600

/*struct and array declarations*/ /*System Variables Declaration*/
struct intf_sys_t
{
    int server_fd;
    vlc_thread_t thread;
    vlc_medialibrary_t *vlc_medialibrary;
    vlc_playlist_t *vlc_playlist;
    vlc_player_t *vlc_player;
};

typedef struct commandFunc
{
    const char *command;
    char* (*commandName)(intf_thread_t *intf, char* arguments);/*some-variable for passing*/ //i think it would be better to pass an array of arguments
}commandFunc;

/*Function Declarations*/
void* clientHandling(void *arg);
static int string_compare_function(const void *key, const void *arrayElement);
commandFunc* searchCommand(const char* key);
void trim(char* s);
char* listall(intf_thread_t *intfa, char* arguments);
/*handler function declarations*/
/* //player commands
char* consume(intf_thread_t *intfa, char* arguments);
char* crossfade(intf_thread_t *intfa, char* arguments);
char* current(intf_thread_t *intfa, char* arguments);
char* queued(intf_thread_t *intfa, char* arguments);
char* mixrampdb(intf_thread_t *intfa, char* arguments);
char* mixrampdelay(intf_thread_t *intfa, char* arguments);
char* next(intf_thread_t *intfa, char* arguments);
char* pause(intf_thread_t *intfa, char* arguments);
char* play(intf_thread_t *intfa, char* arguments);
char* prev(intf_thread_t *intfa, char* arguments);
char* random(intf_thread_t *intfa, char* arguments);
char* repeat(intf_thread_t *intfa, char* arguments);
char* replaygain(intf_thread_t *intfa, char* arguments);
char* single(intf_thread_t *intfa, char* arguments);
char* seek(intf_thread_t *intfa, char* arguments);
char* seekthrough(intf_thread_t *intfa, char* arguments);
char* stop(intf_thread_t *intfa, char* arguments);
char* toggle(intf_thread_t *intfa, char* arguments);

//queue commands - add delete etc
char* add(intf_thread_t *intfa, char* arguments);
char* insert(intf_thread_t *intfa, char* arguments);
char* clear(intf_thread_t *intfa, char* arguments);
char* crop(intf_thread_t *intfa, char* arguments);
char* del(intf_thread_t *intfa, char* arguments);
char* mv(intf_thread_t *intfa, char* arguments);
char* move(intf_thread_t *intfa, char* arguments);
char* searchplay(intf_thread_t *intfa, char* arguments);
char* shuffle(intf_thread_t *intfa, char* arguments);

//database commands
char* listall(intf_thread_t *intfa, char* arguments);
char* ls(intf_thread_t *intfa, char* arguments);
char* search(intf_thread_t *intfa, char* arguments);
char* search(intf_thread_t *intfa, char* arguments);
char* find(intf_thread_t *intfa, char* arguments);
char* findadd(intf_thread_t *intfa, char* arguments);
char* list(intf_thread_t *intfa, char* arguments);
char* stats(intf_thread_t *intfa, char* arguments);
char* update(intf_thread_t *intfa, char* arguments);
char* rescan(intf_thread_t *intfa, char* arguments);

//playlist commands
char* load(intf_thread_t *intfa, char* arguments);
char* lsplaylists(intf_thread_t *intfa, char* arguments);
char* playlist(intf_thread_t *intfa, char* arguments);
char* rm(intf_thread_t *intfa, char* arguments);
char* save(intf_thread_t *intfa, char* arguments);

//output commands
char* volume(intf_thread_t *intfa, char* arguments);
char* outputs(intf_thread_t *intfa, char* arguments);
char* disable(intf_thread_t *intfa, char* arguments);
char* enable(intf_thread_t *intfa, char* arguments);
char* toggleoutput(intf_thread_t *intfa, char* arguments);

//other commands ->idle, noidle
char* idle(intf_thread_t *intfa, char* arguments);
char* noidle(intf_thread_t *intfa, char* arguments);
*/
/*command List Array*/
const commandFunc command_list_array[]={
    {"listall", listall}

    /*{ "add",add },
	{ "addid", addid },
	{ "addtagid", addtagid },
	{ "albumart", album_art },
	//{ "channels", channels },
	{ "clear", clear },
	{ "clearerror", clearerror },
	{ "cleartagid", cleartagid },
	{ "close", close },
	{ "commands", commands },
	{ "config", config },
	{ "consume", consume },
	{ "count", count },
	{ "crossfade", crossfade },
	{ "currentsong", currentsong },
	{ "decoders", decoders },
	{ "delete", delete },
	{ "deleteid", deleteid },
	//{ "delpartition", delpartition },
	{ "disableoutput", disableoutput },
	{ "enableoutput", enableoutput },
	{ "find", find },
	{ "findadd", findadd},
	//{ "getfingerprint", getfingerprint },
	{ "idle", idle },
	{ "kill", kill },
	{ "list", list },
	{ "listall", listall },
	{ "listallinfo", listallinfo },
	{ "listfiles", listfiles },
	//{ "listmounts", listmounts },
	//{ "listneighbors", listneighbors },
	//{ "listpartitions", listpartitions },
	{ "listplaylist", listplaylist },
	{ "listplaylistinfo", listplaylistinfo },
	{ "listplaylists", listplaylists },
	{ "load", load },
	{ "lsinfo", lsinfo },
	{ "mixrampdb", mixrampdb },
	{ "mixrampdelay", mixrampdelay },
	//{ "mount", mount },
	{ "move", move },
	{ "moveid", moveid },
	{ "moveoutput", moveoutput },
	//{ "newpartition", newpartition },
	{ "next", next },
	{ "notcommands", not_commands },
	{ "outputs", devices },
	{ "outputset", outputset },
	//{ "partition", partition },
	//{ "password", password },
	{ "pause", pause },
	{ "ping", ping },
	{ "play", play },
	{ "playid", playid },
	{ "playlist", playlist },
	{ "playlistadd", playlistadd },
	{ "playlistclear", playlistclear },
	{ "playlistdelete", playlistdelete },
	{ "playlistfind", playlistfind },
	{ "playlistid", playlistid },
	{ "playlistinfo", playlistinfo },
	{ "playlistmove", playlistmove },
	{ "playlistsearch", playlistsearch },
	{ "plchanges", plchanges },
	{ "plchangesposid", plchangesposid },
	{ "previous", previous },
	{ "prio", prio },
	{ "prioid", prioid },
	{ "random", random },
	{ "rangeid", rangeid },
	{ "readcomments", read_comments },
	//{ "readmessages", read_messages },
	//{ "readpicture", read_picture },
	{ "rename", rename },
	{ "repeat", repeat },
	{ "replay_gain_mode", replay_gain_mode },
	{ "replay_gain_status", replay_gain_status },
	{ "rescan", rescan },
	{ "rm", rm },
	{ "save", save },
	{ "search", search },
	{ "searchadd", searchadd },
	{ "searchaddpl", searchaddpl },
	{ "seek", seek },
	{ "seekcur", seekcur },
	{ "seekid", seekid },
	//{ "sendmessage", send_message },
	{ "setvol", setvol },
	{ "shuffle", shuffle },
	{ "single", single },
	{ "stats", stats },
	{ "status", status },
	//{ "sticker", sticker },
	{ "stop", stop },
	//{ "subscribe", subscribe },
	{ "swap", swap },
	{ "swapid", swapid },
	{ "tagtypes", tagtypes },
	{ "toggleoutput", toggleoutput },
	//{ "unmount", unmount },
	//{ "unsubscribe", unsubscribe },
	{ "update", handle_update },
	{ "urlhandlers", urlhandlers },
	{ "volume", volume }*/
};

/*command functions - main media playback here*/
//player commands
char* consume(intf_thread_t *intfa, char* arguments){}
char* crossfade(intf_thread_t *intfa, char* arguments){}
char* current(intf_thread_t *intfa, char* arguments){}
char* queued(intf_thread_t *intfa, char* arguments){}
char* mixrampdb(intf_thread_t *intfa, char* arguments){}
char* mixrampdelay(intf_thread_t *intfa, char* arguments){}
char* next(intf_thread_t *intfa, char* arguments){}
char* pauseP(intf_thread_t *intfa, char* arguments){}
char* play(intf_thread_t *intfa, char* arguments){
    // //playerInit
    // vlc_player_t *player=intfa->p_sys->vlc_player;
    // vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    // input_item_t *media=input_item_New("file:///home/nightwayne/Music/Natural.mp3","N"); //URI file path error
    
    // //playing1
    // int t;
    // vlc_player_Lock(player);
    // t=vlc_player_SetCurrentMedia(player, media);
    // t=vlc_player_Start(player);
    // sleep(7);
    // vlc_player_Stop(player);
    // vlc_player_Unlock(player);
    // input_item_Release (media);
    
    //playing2
    // vlc_playlist_Lock(playlist);
    // t=vlc_playlist_InsertOne(playlist,0,media);
    // t=vlc_playlist_Start(playlist);
    // vlc_playlist_Unlock(playlist);
    // input_item_Release (media);
}
char* prev(intf_thread_t *intfa, char* arguments){}
char* randomP(intf_thread_t *intfa, char* arguments){}//https://www.videolan.org/developers/vlc/doc/doxygen/html/group__playlist__randomizer.html
char* repeat(intf_thread_t *intfa, char* arguments){}
char* replaygain(intf_thread_t *intfa, char* arguments){}
char* single(intf_thread_t *intfa, char* arguments){}
char* seek(intf_thread_t *intfa, char* arguments){}
char* seekthrough(intf_thread_t *intfa, char* arguments){}
char* stop(intf_thread_t *intfa, char* arguments){}
char* toggle(intf_thread_t *intfa, char* arguments){}

//queue commands - add delete etc
char* add(intf_thread_t *intfa, char* arguments){}
char* insert(intf_thread_t *intfa, char* arguments){}
char* clear(intf_thread_t *intfa, char* arguments){}
char* crop(intf_thread_t *intfa, char* arguments){}
char* del(intf_thread_t *intfa, char* arguments){}
char* mv(intf_thread_t *intfa, char* arguments){}
char* move(intf_thread_t *intfa, char* arguments){}
char* searchplay(intf_thread_t *intfa, char* arguments){}
char* shuffle(intf_thread_t *intfa, char* arguments){}

//database commands
char* listall(intf_thread_t *intfa, char* arguments)
{
    printf("listAll\n");
    char *returnString=malloc(sizeof(char)*4096);
    vlc_medialibrary_t* p_ml = intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t params = vlc_ml_query_params_create(); vlc_ml_query_params_t* p_params = &params;

    vlc_ml_media_list_t *p_vlc_ml_media_list_t=vlc_ml_list_audio_media (p_ml, p_params);
    char *title, *c;
    for(int i=0;i<p_vlc_ml_media_list_t->i_nb_items;i++)
    {
        vlc_ml_media_t *p_vlc_ml_media_t=&p_vlc_ml_media_list_t->p_items[i];
        title=p_vlc_ml_media_t->psz_title;
        c=strcat(title,"\n");
        //msg_Info(intfa,"%s",title);
        c=strcat(returnString,title);
        //msg_Info(intfa,"%s",c);
        //vlc_ml_media_release (p_vlc_ml_media_t);
    }
    //vlc_ml_media_list_release(p_vlc_ml_media_list_t);
    c=strcat(returnString,"\n");
    //msg_Info(intfa,"%s\n%s\n%s\n",title,c,returnString);
    return returnString;
}
char* ls(intf_thread_t *intfa, char* arguments){}
char* search(intf_thread_t *intfa, char* arguments){}
char* searchExp(intf_thread_t *intfa, char* arguments){}
char* find(intf_thread_t *intfa, char* arguments){}
char* findadd(intf_thread_t *intfa, char* arguments){}
char* list(intf_thread_t *intfa, char* arguments){}
char* stats(intf_thread_t *intfa, char* arguments){}
char* update(intf_thread_t *intfa, char* arguments){}
char* rescan(intf_thread_t *intfa, char* arguments){}

//playlist commands
char* load(intf_thread_t *intfa, char* arguments){}
char* lsplaylists(intf_thread_t *intfa, char* arguments){}
char* playlistP(intf_thread_t *intfa, char* arguments){}
char* rm(intf_thread_t *intfa, char* arguments){}
char* save(intf_thread_t *intfa, char* arguments){}

//output commands
char* volume(intf_thread_t *intfa, char* arguments){}
char* outputs(intf_thread_t *intfa, char* arguments){}
char* disable(intf_thread_t *intfa, char* arguments){}
char* enable(intf_thread_t *intfa, char* arguments){}
char* toggleoutput(intf_thread_t *intfa, char* arguments){}

//other commands ->idle, noidle
char* idle(intf_thread_t *intfa, char* arguments){}
char* noidle(intf_thread_t *intfa, char* arguments){}

//mount commands -> no idea
//sticker commands -> not to be done
//client to client commands -> not to be done


static int string_compare_function(const void *key, const void *arrayElement)
{
    const commandFunc *commandFunc = arrayElement;
    return strcmp(key, commandFunc->command);/*each list item's string binary search*/
}
commandFunc* searchCommand(const char* key)
{
    return bsearch(key, command_list_array, ARRAY_SIZE(command_list_array),sizeof(*command_list_array), string_compare_function);
}
void trim(char *s)
{
	int  i,j;
 
	for(i=0;s[i]==' '||s[i]=='\t';i++);
		
	for(j=0;s[i];i++)
	{
		s[j++]=s[i];
	}
	s[j]='\0';
	for(i=0;s[i]!='\0';i++)
	{
		if(s[i]!=' '&& s[i]!='\t')
				j=i;
	}
	s[j+1]='\0';
}

/*client Handling thread*/
void* clientHandling(void *threadArg) //Polling + Client Req Handling
{
    // 0. Function Start
    intf_thread_t *intfa = (intf_thread_t *)threadArg;
    msg_Info(intfa, "Hello from thread side!");

    // 1. Initialising Client
    int server_fd = intfa->p_sys->server_fd;
    struct sockaddr_in clientAddr;
    int client_fd,clientAddrLen = sizeof(clientAddr);
    
    //file desciptor sets
    fd_set currentSockets,readySockets;
    FD_ZERO(&currentSockets);
    FD_ZERO(&readySockets);
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
        if(select(FD_SETSIZE, &readySockets,NULL,NULL,NULL)<0)
        {
            // Accepting(Monitoring) Multiple CLients using Select() method for polling
            //readySockets has read operation pending
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if(FD_ISSET(i,&readySockets))
            {
                if(i==server_fd) 
                {
                    // 0. server has new client connection
                    bzero((char *)&clientAddr, sizeof(clientAddr));
                    if ((client_fd = accept(server_fd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientAddrLen)) < 0)
                    {
                        perror("accept");   
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(client_fd,&currentSockets);  //move client_fd to current
                    
                    //medialibraryInit
                    // commandFunc* command=&command_list_array[0];
                    // char *out = command->commandName(intfa, "arg"); //correct way of recieving char* from function
                    // msg_Info(intfa,"%s",out);
                    // free(out);  //clear memory allocation.
                    
                    // 1. return OK connection established
                    char *output="OK MPD Version 0.21.25";
                    send(client_fd, output, strlen(output), 0);
                    msg_Info(intfa, "server read: %d\n", client_fd);
                }
                else 
                {
                    // 0. client has pending read connection (new message)
                    client_fd = i;
                    msg_Info(intfa, "client_fd read: %d\n", client_fd);
                    
                    // 1. read operation ->read data of unknown length - ask mentors
                    int n; 
                    char *input=malloc(sizeof(char)*4096), *arguments=malloc(sizeof(char)*4096), *command=malloc(sizeof(char)*4096);
                    bzero(input,4096); bzero(arguments,4096);
                    if ((n = read(client_fd,input,4095)) < 0)
                    {
                        perror("read");
                        exit(EXIT_FAILURE);
                    }
                    input[strlen(input)-1]='\0';
                    
                    // 2. Size 0 input -handled
                    if(strlen(input)==0)
                    {
                        //continue; ->if continued 0 length input, then needs new approach.
                        char* arguments="arg";
                        commandFunc* command=searchCommand("listall");//or use "commands"
                        char *output = command->commandName(intfa, arguments); 
                        msg_Info(intfa,"Size0 %s",output);
                        send(client_fd, output, strlen(output), 0);
                        free(output);
                    }
                
                    // 3. remove whitespace in left and right ->check
                    trim(input);
                    
                    // 4. separate command and argument using whitespace
                    arguments=strchr(input,' ');
                    char *acommand;
                    if(arguments!=NULL)
                    {
                        int s=strlen(input);
                        strncat(command,input,s-strlen(arguments));
                        trim(arguments);
                    }
                    else
                    arguments="";
                    //msg_Info(intfa, "input:%s.\ninputSize:%ld.\n\narguments:%s.\nargumentsSize:%ld.\n",command,strlen(command),arguments,strlen(arguments)); 
                    
                    // 5. check for lowercase - here I am explicity converting to small string
                    tolower(command);

                    // 6. search for the command in the array
                    commandFunc* commandF=searchCommand(input);
                    if(commandF==NULL)
                    {
                        //study the ACK.hxx & follow up
                        msg_Info(intfa,"command not found\n");
                        send(client_fd, "OK\n", strlen("OK\n"), 0);
                        //send(client_fd, "ACK command not found\n", strlen("ACK command not found\n"), 0);
                        FD_CLR(client_fd, &currentSockets); //remove client_fd to current
                    }
                    else
                    {
                        char* output = commandF->commandName(intfa, arguments); 
                        send(client_fd, output, strlen(output), 0);
                        free(output);
                    }
                    free(input);
                    free(command);
                    free(arguments);
                    FD_CLR(client_fd, &currentSockets); //remove client_fd to current
                }
            }
        }//for
    }//while

    // 4. Function End
    return NULL;
}

/*Open Module*/
static int Open(vlc_object_t *obj)
{
    // 0. Module Start 
    intf_thread_t *intfa = (intf_thread_t *)obj; //thread for interface debugging
    msg_Info(intfa, "Hello World! MPD Server Started");

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
    intf_thread_t *intf = (intf_thread_t *)obj; //thread for interface debugging
    vlc_medialibrary_t* p_ml=vlc_ml_instance_get(obj);
    vlc_playlist_t *playlist=vlc_intf_GetMainPlaylist(intfa);
    vlc_player_t *player=vlc_playlist_GetPlayer(playlist);
    // vlc_playlist_t* playlist=vlc_playlist_New(obj);
    // vlc_player_t *player=vlc_player_New(obj,VLC_PLAYER_LOCK_NORMAL,NULL,NULL);

    intf_sys_t *sys = malloc(sizeof(*sys)); //intf_sys_t *sys is for (storing/passing system variables) internal state.
    if (unlikely(sys == NULL))
        return VLC_ENOMEM;
    
    intf->p_sys=sys; //p_sys is of type p_sys;
    sys->server_fd=server_fd;
    sys->vlc_medialibrary=p_ml;
    sys->vlc_playlist=/*vlc_intf_GetMainPlaylist(intfa);*/playlist;
    sys->vlc_player=/*vlc_playlist_GetPlayer(sys->vlc_playlist);*/player;
    
    // 6. Client Handling
    int i=0;
    if((i = vlc_clone(/*&intf->p_*/&sys->thread, clientHandling, intf, 0))<0) //0 - VLC_THREAD_PRIORITY_LOW
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
    // -those dynamically allocated, not static allocation (like int i = 10;)
    // check for appropriate dynamic allocation, I have used only static allocation
    intf_sys_t *sys = intf->p_sys;
    vlc_cancel(intf->p_sys->thread);
    vlc_join(intf->p_sys->thread, NULL);
    // free(sys->vlc_medialibrary);
    // free(sys->vlc_playlist);
    // free(sys->vlc_player);
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
