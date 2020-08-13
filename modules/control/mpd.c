#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#pragma region
/*Standard Header Files*/
#include <stdlib.h>
#include <ctype.h>
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
#include <vlc_mpd.h>
#include <vlc_media_library.h>
#include <vlc_player.h>
#include <vlc_playlist.h>
#include <vlc_playlist_export.h>
#include <vlc_url.h>
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_input_item.h>
#include <vlc_aout.h>
#include <vlc_vout.h>
#include <vlc_actions.h>
/* Macros - should make them configurable?*/
#define PORT 6600
#define STATUS_CHANGE "status change: "

/* System Variables, Struct and Vector Declaration */
struct intf_sys_t
{
    /* common function variables */
    int server_fd;
    vlc_thread_t thread;

    /* mpd variables */
    vlc_medialibrary_t *vlc_medialibrary;
    vlc_playlist_t *vlc_playlist;
    //vlc_player_t *vlc_player;

    /* player listeners */
    vlc_player_listener_id *player_listener;
    //vlc_player_aout_listener_id *player_aout_listener;

    /* status changes */
    vlc_mutex_t status_lock;
    enum vlc_player_state last_state;
    //bool b_input_buffering;
};
typedef struct commandFunc
{
    const char *command;
    char *(*commandName)(intf_thread_t *intf, char *arguments);
} commandFunc;
typedef struct VLC_VECTOR(char*) vect;

// Function Declaration
void reverse(char str[], int length);
char* itoaFunction(int num, char* str, int base);
int atoiFunction(char* str);
static int stringCompare(const void *key, const void *arrayElement);
commandFunc *searchCommand(const char *key);
void trim(char *s);
void parseInput(intf_thread_t *intfa, char *input, char *command, char *argumentsC);
void getArg(intf_thread_t *intfa, char* str,vect* v);
void destroyVector(vect* v);
void *clientHandling(void *arg);
static void player_on_state_changed(vlc_player_t *player, enum vlc_player_state state, void *data);
#pragma endregion

/* MPD Interaction Commands Not Available */
#pragma region
/* Command Array */
const commandFunc command_list_array[] = {
    //{"listall", listall}
    {"add", add},
    {"addid", addid},
    {"addtagid", addtagid},
    {"albumart", album_art},
    {"channels", channels},
    {"clear", clear},
    {"clearerror", clearerror},
    {"cleartagid", cleartagid},
    {"close", closeF},
    {"commands", commands},
    {"config", config},
    {"consume", consume},
    {"count", count},
    {"crossfade", crossfade},
    {"currentsong", currentsong},
    {"decoders", decoders},
    {"delete", deleteF},
    {"deleteid", deleteid},
    {"delpartition", delpartition},
    {"disableoutput", disableoutput},
    {"enableoutput", enableoutput},
    {"find", find},
    {"findadd", findadd},
    {"getfingerprint", getfingerprint},
    {"idle", idle},
    {"kill", kill},
    {"list", list},
    {"listall", listall},
    {"listallinfo", listallinfo},
    {"listfiles", listfiles},
    {"listmounts", listmounts},
    {"listneighbors", listneighbors},
    {"listpartitions", listpartitions},
    {"listplaylist", listplaylist},
    {"listplaylistinfo", listplaylistinfo},
    {"listplaylists", listplaylists},
    {"load", load},
    {"lsinfo", lsinfo},
    {"mixrampdb", mixrampdb},
    {"mixrampdelay", mixrampdelay},
    {"mount", mount},
    {"move", move},
    {"moveid", moveid},
    {"moveoutput", moveoutput},
    {"newpartition", newpartition},
    {"next", next},
    {"notcommands", not_commands},
    {"outputs", devices},
    {"outputset", outputset},
    {"partition", partition},
    {"password", password},
    {"pause", pauseF},
    {"ping", ping},
    {"play", play},
    {"playid", playid},
    {"playlist", playlist},
    {"playlistadd", playlistadd},
    {"playlistclear", playlistclear},
    {"playlistdelete", playlistdelete},
    {"playlistfind", playlistfind},
    {"playlistid", playlistid},
    {"playlistinfo", playlistinfo},
    {"playlistmove", playlistmove},
    {"playlistsearch", playlistsearch},
    {"plchanges", plchanges},
    {"plchangesposid", plchangesposid},
    {"previous", previous},
    {"prio", prio},
    {"prioid", prioid},
    {"random", randomF},
    {"rangeid", rangeid},
    {"readcomments", read_comments},
    {"readmessages", read_messages},
    {"readpicture", read_picture},
    {"rename", renameF},
    {"repeat", repeat},
    {"replay_gain_mode", replay_gain_mode},
    {"replay_gain_status", replay_gain_status},
    {"rescan", rescan},
    {"rm", rm},
    {"save", save},
    {"search", search},
    {"searchadd", searchadd},
    {"searchaddpl", searchaddpl},
    {"seek", seek},
    {"seekcur", seekcur},
    {"seekid", seekid},
    {"sendmessage", send_message},
    {"setvol", setvol},
    {"shuffle", shuffle},
    {"single", single},
    {"stats", stats},
    {"status", status},
    {"sticker", sticker},
    {"stop", stop},
    {"subscribe", subscribe},
    {"swap", swap},
    {"swapid", swapid},
    {"tagtypes", tagtypes},
    {"toggleoutput", toggleoutput},
    {"unmount", unmount},
    {"unsubscribe", unsubscribe},
    {"update", handle_update},
    {"urlhandlers", urlhandlers},
    {"volume", volume}};

//file commands - reading music file ??
char *read_comments(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *album_art(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *read_picture(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}

//fingerprint commands
char *getfingerprint(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}

//Message c2c communication
char *subscribe(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *unsubscribe(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *channels(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *read_messages(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *send_message(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}

//neighbour commands
char *listneighbors(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}

//partition commands
char *partition(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *listpartitions(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *newpartition(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *delpartition(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *moveoutput(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}

//sticker commands
char *sticker(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}

//storage commands
char *listmounts(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *mount(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *unmount(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}

//tag commands ->volatile changes ->how to add them dynamically without modifying the song(media) file
char *addtagid(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *cleartagid(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
#pragma endregion

/* MPD Interaction Commands Available */
#pragma region
//player commands
char *play(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_player_Lock(player);
    vlc_playlist_item_t *playlistItem= vlc_playlist_Get(playlist,v.data[0]);
    input_item_t *item=vlc_playlist_item_GetMedia (playlistItem);
    vlc_player_SetCurrentMedia(player, item);
    vlc_player_Start(player);
    vlc_player_Unlock(player);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "Play Pos%s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *playid(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_player_Lock(player);
    size_t ind=vlc_playlist_IndexOfId(playlist, v.data[0]);
    vlc_playlist_item_t *playlistItem= vlc_playlist_Get(playlist,ind);
    input_item_t *item=vlc_playlist_item_GetMedia (playlistItem);
    vlc_player_SetCurrentMedia(player, item);
    vlc_player_Start(player);
    vlc_player_Unlock(player);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "Play Id%s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *stop(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_player_Lock(player);
    vlc_player_Stop(player);
    vlc_player_Unlock(player);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "Play Pos%s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *currentsong(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_player_Lock(player);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    input_item_t *item = vlc_player_GetCurrentMedia(player);
    //input_item_t* item= vlc_playlist_item_GetMedia(vlc_playlist_Get(playlist,i)); //check if they give same results
    strcat(output,item->psz_name);
    strcat(output,"\n");
    vlc_player_Unlock(player);
    strcat(output, "OK\n");
    msg_Info(intfa, "Play Pos%s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *pauseF(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    //add pause argument handling 0 and 1;
    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_player_Lock(player);
    vlc_player_Pause(player);
    vlc_player_Unlock(player);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "Play Pos%s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *status(intf_thread_t *intfa, char *arguments) //need to look at all state changes and report then
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    char *output = malloc(sizeof(char) * 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "Play Pos%s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *next(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_player_Lock(player);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    if(!vlc_playlist_HasNext(playlist))
    {
        strcat(output, "ACK end of playlist\n");
    }
    vlc_playlist_Next(playlist);
    size_t ind=vlc_playlist_GetCurrentIndex(playlist);
    vlc_playlist_item_t *playlistItem= vlc_playlist_Get(playlist,ind);
    input_item_t *item=vlc_playlist_item_GetMedia (playlistItem);
    if(vlc_player_IsStarted(player))
    vlc_player_Stop(player);
    
    vlc_player_SetCurrentMedia(player, item);
    vlc_player_Start(player);
    vlc_player_Unlock(player);
    strcat(output, "OK\n");
    msg_Info(intfa, "Next%s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *previous(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_player_Lock(player);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    if(!vlc_playlist_HasPrev(playlist))
    {
        strcat(output, "ACK start of playlist\n");
    }
    vlc_playlist_Prev(playlist);
    size_t ind=vlc_playlist_GetCurrentIndex(playlist);
    vlc_playlist_item_t *playlistItem= vlc_playlist_Get(playlist,ind);
    input_item_t *item=vlc_playlist_item_GetMedia (playlistItem);
    if(vlc_player_IsStarted(player))
    vlc_player_Stop(player);
    
    vlc_player_SetCurrentMedia(player, item);
    vlc_player_Start(player);
    vlc_player_Unlock(player);
    strcat(output, "OK\n");
    msg_Info(intfa, "Next%s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *repeat(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);

    //add repeat argument handling 0 and 1;
    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_player_Lock(player);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    if(v.data[0]=="0")
    {
        enum vlc_playlist_playback_repeat mode=VLC_PLAYLIST_PLAYBACK_REPEAT_NONE;
        vlc_playlist_SetPlaybackRepeat (playlist, mode);
        strcat(output, "Repeat 0\nOK\n");   
    }
    else
    {
        enum vlc_playlist_playback_repeat mode=VLC_PLAYLIST_PLAYBACK_REPEAT_CURRENT;
        vlc_playlist_SetPlaybackRepeat (playlist, mode);
        strcat(output, "Repeat 1\nOK\n");   
    }
    vlc_player_Unlock(player);
    msg_Info(intfa, "Play Pos%s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *single(intf_thread_t *intfa, char *arguments) //how to stop playback after completing song
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *consume(intf_thread_t *intfa, char *arguments)//how to stop playback after completing song 
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *randomF(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);

    //add repeat argument handling 0 and 1;
    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_player_Lock(player);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    if(v.data[0]=="0")
    {
        enum vlc_playlist_playback_order mode=VLC_PLAYLIST_PLAYBACK_ORDER_NORMAL;
        vlc_playlist_SetPlaybackOrder (playlist, mode);
        strcat(output, "Random 0\nOK\n");   
    }
    else
    {
        enum vlc_playlist_playback_order mode=VLC_PLAYLIST_PLAYBACK_ORDER_RANDOM;
        vlc_playlist_SetPlaybackOrder (playlist, mode);
        strcat(output, "Random 1\nOK\n");   
    }
    vlc_player_Unlock(player);
    msg_Info(intfa, "Play Pos%s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *clearerror(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *seek(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_player_Lock(player);
    vlc_playlist_item_t *playlistItem=vlc_playlist_Get(playlist,v.data[0]);
    input_item_t *item=vlc_playlist_item_GetMedia (playlistItem);
    vlc_tick_t t=atoiFunction(v.data[1]);
    enum vlc_player_seek_speed speed=VLC_PLAYER_SEEK_FAST;
    enum vlc_player_whence whence=VLC_PLAYER_WHENCE_ABSOLUTE;
    vlc_player_SetCurrentMedia(player, item);
    vlc_player_SeekByTime (player,t,speed,whence);
    vlc_player_Start(player);
    vlc_player_Unlock(player);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "Seek Pos%s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *seekid(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_player_Lock(player);
    size_t ind=vlc_playlist_IndexOfId(playlist, v.data[0]);
    vlc_playlist_item_t *playlistItem= vlc_playlist_Get(playlist,ind);
    input_item_t *item=vlc_playlist_item_GetMedia (playlistItem);
    vlc_tick_t t=atoiFunction(v.data[1]);
    enum vlc_player_seek_speed speed=VLC_PLAYER_SEEK_FAST;
    enum vlc_player_whence whence=VLC_PLAYER_WHENCE_ABSOLUTE;
    vlc_player_SetCurrentMedia(player, item);
    vlc_player_SeekByTime (player,t,speed,whence);
    vlc_player_Start(player);
    vlc_player_Unlock(player);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "Seek ID%s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *seekcur(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_player_Lock(player);
    vlc_tick_t t=atoiFunction(v.data[1]);
    enum vlc_player_seek_speed speed=VLC_PLAYER_SEEK_FAST;
    enum vlc_player_whence whence=VLC_PLAYER_WHENCE_ABSOLUTE;
    if(vlc_player_IsStarted(player))
    vlc_player_Stop(player);
    vlc_player_SeekByTime (player,t,speed,whence);
    vlc_player_Start(player);
    vlc_player_Unlock(player);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "Seek Pos%s.", output);
    destroyVector(&v);
    return (char *)output;
}
//not available in VLC as per my knowledge
char *crossfade(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *mixrampdb(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *mixrampdelay(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *replay_gain_mode(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *replay_gain_status(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}

//queue commands
char *add(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    //assuming for now only single uri, not folder
    char* inp=vlc_path2uri(v.data[0],"file");
    input_item_t *media=input_item_New(inp,"wit");
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    size_t ind=vlc_playlist_Count(playlist);
    vlc_playlist_InsertOne(playlist,ind,media);
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "Add %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *addid(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    //assuming for now only single uri, not folder
    char* inp=vlc_path2uri(v.data[0],"file");
    input_item_t *media=input_item_New(inp,"wit");
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    size_t ind=vlc_playlist_Count(playlist);
    vlc_playlist_InsertOne(playlist,ind,media);
    vlc_playlist_item_t *item = vlc_playlist_Get (playlist,ind);
    int id=vlc_playlist_item_GetId(item);
    char *s=NULL; s=itoaFunction(id,s,10);
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output,"ID: ");  strcat(output,s);   strcat(output, "OK\n");
    msg_Info(intfa, "AddID %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *rangeid(intf_thread_t *intfa, char *arguments)//range of a song
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *deleteF(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    //assuming for now only single uri, not folder
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    vlc_playlist_RemoveOne (playlist,v.data[0]);
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "DeletePos %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *deleteid(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    //assuming for now only single uri, not folder
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    int id=atoiFunction(v.data[0]);
    size_t ind=vlc_playlist_IndexOfId(playlist, ind);
    vlc_playlist_RemoveOne (playlist,ind);
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "DeleteId %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *playlist(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    //assuming for now only single uri, not folder
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    for(int i=0;i<vlc_playlist_Count(playlist);i++)
    {
        input_item_t* item= vlc_playlist_item_GetMedia(vlc_playlist_Get(playlist,i));
        strcat(output,item->psz_name);
        strcat(output,"\n");
    }
    strcat(output, "OK\n");
    vlc_playlist_Unlock(playlist);
    msg_Info(intfa, "Playlist Display %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *shuffle(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    //assuming for now only single uri, not folder
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    vlc_playlist_Shuffle(playlist);
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "Shuffle %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *clear(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    //assuming for now only single uri, not folder
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    vlc_playlist_Clear(playlist);
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "Clear %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *plchanges(intf_thread_t *intfa, char *arguments)//Didn't understand on the run update feature .m3u related
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *plchangesposid(intf_thread_t *intfa, char *arguments)//Didn't understand on the run update feature .m3u related
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playlistinfo(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    //assuming for now only single uri, not folder
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    for(int i=0;i<vlc_playlist_Count(playlist);i++)
    {
        input_item_t* item= vlc_playlist_item_GetMedia(vlc_playlist_Get(playlist,i));
        strcat(output,item->psz_name);
        strcat(output,"\n");
    }
    strcat(output, "OK\n");
    vlc_playlist_Unlock(playlist);
    msg_Info(intfa, "Playlist Display Info %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *playlistid(intf_thread_t *intfa, char *arguments)
{
   vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    

    //assuming for now songid is given here
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    size_t ind=vlc_playlist_IndexOfId(playlist, v.data[0]);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    input_item_t* item= vlc_playlist_item_GetMedia(vlc_playlist_Get(playlist,ind));
    strcat(output,item->psz_name); strcat(output,"\n");
    strcat(output, "OK\n");
    vlc_playlist_Unlock(playlist);
    msg_Info(intfa, "Playlist Display Info %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *playlistfind(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playlistsearch(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *prio(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *prioid(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *move(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    //assuming for now only single uri, not folder
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    //add checks to and from within range
    vlc_playlist_Lock(playlist);
    // size_t ind1=vlc_playlist_IndexOfId(playlist, v.data[0]);
    // size_t ind2=vlc_playlist_IndexOfId(playlist, v.data[1]);
    int bef=atoiFunction(v.data[0]),aft=atoiFunction(v.data[1]);
    if(v.data[0]>v.data[1])
    {
        aft=v.data[0];  bef=v.data[1];
    }
    vlc_playlist_item_t *item1=vlc_playlist_Get(playlist, bef);
    vlc_playlist_RemoveOne(playlist,bef);
    vlc_playlist_item_t *item2=vlc_playlist_Get(playlist, aft-1);
    vlc_playlist_RemoveOne(playlist,aft);
    vlc_playlist_InsertOne (playlist,bef,item2);
    vlc_playlist_InsertOne (playlist,aft,item1);
    //can also use temp elem swap here - think that would be better.
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "MoveId %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *moveid(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    //assuming for now only single uri, not folder
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    //add checks to and from within range
    vlc_playlist_Lock(playlist);
    v.data[0]=atoiFunction(v.data[0]);v.data[1]=atoiFunction(v.data[1]);
    size_t ind1=vlc_playlist_IndexOfId(playlist, v.data[0]);
    size_t ind2=vlc_playlist_IndexOfId(playlist, v.data[1]);
    int bef=ind1,aft=ind2;
    if(v.data[0]>v.data[1])
    {
        aft=v.data[0];  bef=v.data[1];
    }
    vlc_playlist_item_t *item1=vlc_playlist_Get(playlist, bef);
    vlc_playlist_RemoveOne(playlist,bef);
    vlc_playlist_item_t *item2=vlc_playlist_Get(playlist, aft-1);
    vlc_playlist_RemoveOne(playlist,aft);
    vlc_playlist_InsertOne (playlist,bef,item2);
    vlc_playlist_InsertOne (playlist,aft,item1);
    //can also use temp elem swap here - think that would be better.
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "DeleteId %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *swap(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    //assuming for now only single uri, not folder
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    //add checks to and from within range
    vlc_playlist_Lock(playlist);
    // size_t ind1=vlc_playlist_IndexOfId(playlist, v.data[0]);
    // size_t ind2=vlc_playlist_IndexOfId(playlist, v.data[1]);
    int bef=atoiFunction(v.data[0]),aft=atoiFunction(v.data[1]);
    if(v.data[0]>v.data[1])
    {
        aft=v.data[0];  bef=v.data[1];
    }
    vlc_playlist_item_t *item1=vlc_playlist_Get(playlist, bef);
    vlc_playlist_RemoveOne(playlist,bef);
    vlc_playlist_item_t *item2=vlc_playlist_Get(playlist, aft-1);
    vlc_playlist_RemoveOne(playlist,aft);
    vlc_playlist_InsertOne (playlist,bef,item2);
    vlc_playlist_InsertOne (playlist,aft,item1);
    //can also use temp elem swap here - think that would be better.
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "DeleteId %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *swapid(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    //assuming for now only single uri, not folder
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    //add checks to and from within range
    vlc_playlist_Lock(playlist);
    v.data[0]=atoiFunction(v.data[0]);v.data[1]=atoiFunction(v.data[1]);// i think error in all these because v.data[0] is of char type
    size_t ind1=vlc_playlist_IndexOfId(playlist, v.data[0]);
    size_t ind2=vlc_playlist_IndexOfId(playlist, v.data[1]);
    int bef=ind1,aft=ind2;
    if(v.data[0]>v.data[1])
    {
        aft=v.data[0];  bef=v.data[1];
    }
    vlc_playlist_item_t *item1=vlc_playlist_Get(playlist, bef);
    vlc_playlist_RemoveOne(playlist,bef);
    vlc_playlist_item_t *item2=vlc_playlist_Get(playlist, aft-1);
    vlc_playlist_RemoveOne(playlist,aft);
    vlc_playlist_InsertOne (playlist,bef,item2);
    vlc_playlist_InsertOne (playlist,aft,item1);
    //can also use temp elem swap here - think that would be better.
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "DeleteId %s.", output);
    destroyVector(&v);
    return (char *)output;
}

//database commands
char *find(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *findadd(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *search(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *searchadd(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *searchaddpl(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *count(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *listall(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);

    printf("listAll\n");
    char *returnString=malloc(sizeof(char)*4096);
    bzero(returnString,4096);
    vlc_medialibrary_t* p_ml = intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t params = vlc_ml_query_params_create(); vlc_ml_query_params_t* p_params = &params;
    vlc_ml_media_list_t *p_vlc_ml_media_list_t=vlc_ml_list_audio_media (p_ml, p_params);
    char *title=NULL;
    for(int i=0;i<p_vlc_ml_media_list_t->i_nb_items;i++)
    {
        vlc_ml_media_t *p_vlc_ml_media_t=&p_vlc_ml_media_list_t->p_items[i];
        title=p_vlc_ml_media_t->psz_title;
        strcat(title,"\n");
        strcat(returnString,title);
        //release not working
        //vlc_ml_media_release (p_vlc_ml_media_t);
    }
    //vlc_ml_media_list_release(p_vlc_ml_media_list_t);
    msg_Info(intfa,"%s%s.",title,returnString);
    destroyVector(&v);
    return (char*)returnString;
    
}
char *list(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *listallinfo(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}

//playlist commands
char *save(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    char *filename=NULL;
    strcat(filename,"/home/nightwayne/MPD/"); //directory where playlists are saved
    strcat(filename,v.data[0]);
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    vlc_playlist_Export (playlist, filename, ".xspf");
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "Save playlist %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *load(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);   
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    vlc_playlist_Clear(playlist);
    vlc_tick_t t=0;
    enum input_item_type_e i_type=ITEM_TYPE_PLAYLIST;
    enum input_item_net_type i_net=ITEM_NET_UNKNOWN;
    input_item_t * item= input_item_NewExt (v.data[0],"dummy"/*have to finf a way to extract it*/,t, i_type, i_net);
    //need to test it, if media-type=playlist can be added or not;
    vlc_playlist_InsertOne (playlist,0,item);
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "Save playlist %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *listplaylist(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);   
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    vlc_playlist_Clear(playlist);
    vlc_tick_t t=0;
    enum input_item_type_e i_type=ITEM_TYPE_PLAYLIST;
    enum input_item_net_type i_net=ITEM_NET_UNKNOWN;
    input_item_t * item= input_item_NewExt (v.data[0],"dummy"/*have to finf a way to extract it*/,t, i_type, i_net);
    //need to test it, if media-type=playlist can be added or not;
    vlc_playlist_InsertOne (playlist,0,item);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    for(int i=0;i<vlc_playlist_Count(playlist);i++)
    {
        input_item_t* item= vlc_playlist_item_GetMedia(vlc_playlist_Get(playlist,i));
        strcat(output,item->psz_name);
        strcat(output,"\n");
    }
    strcat(output, "OK\n");
    vlc_playlist_Unlock(playlist);
    strcat(output, "OK\n");
    msg_Info(intfa, "Save playlist %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *listplaylistinfo(intf_thread_t *intfa, char *arguments) //need to add meta-data, similar to playlistinfo
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);   
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    vlc_playlist_Clear(playlist);
    vlc_tick_t t=0;
    enum input_item_type_e i_type=ITEM_TYPE_PLAYLIST;
    enum input_item_net_type i_net=ITEM_NET_UNKNOWN;
    input_item_t * item= input_item_NewExt (v.data[0],"dummy"/*have to finf a way to extract it*/,t, i_type, i_net);
    //need to test it, if media-type=playlist can be added or not;
    vlc_playlist_InsertOne (playlist,0,item);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    for(int i=0;i<vlc_playlist_Count(playlist);i++)
    {
        input_item_t* item= vlc_playlist_item_GetMedia(vlc_playlist_Get(playlist,i));
        strcat(output,item->psz_name);
        strcat(output,"\n");
    }
    strcat(output, "OK\n");
    vlc_playlist_Unlock(playlist);
    strcat(output, "OK\n");
    msg_Info(intfa, "Save playlist %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *rm(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *renameF(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playlistdelete(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);   
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    vlc_playlist_Clear(playlist);
    vlc_tick_t t=0;
    enum input_item_type_e i_type=ITEM_TYPE_PLAYLIST;
    enum input_item_net_type i_net=ITEM_NET_UNKNOWN;
    input_item_t * item= input_item_NewExt (v.data[0],"dummy"/*have to finf a way to extract it*/,t, i_type, i_net);
    //need to test it, if media-type=playlist can be added or not;
    vlc_playlist_InsertOne (playlist,0,item);
    int ind=atoiFunction(v.data[1]);
    vlc_playlist_RemoveOne (playlist,ind);
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "Save playlist %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *playlistmove(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playlistclear(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);   
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    vlc_playlist_Clear(playlist);
    vlc_tick_t t=0;
    enum input_item_type_e i_type=ITEM_TYPE_PLAYLIST;
    enum input_item_net_type i_net=ITEM_NET_UNKNOWN;
    input_item_t * item= input_item_NewExt (v.data[0],"dummy"/*have to finf a way to extract it*/,t, i_type, i_net);
    //need to test it, if media-type=playlist can be added or not;
    vlc_playlist_InsertOne (playlist,0,item);
    vlc_playlist_Clear(playlist);
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "Save playlist %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *playlistadd(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);   
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    vlc_playlist_Clear(playlist);
    vlc_tick_t t=0;
    enum input_item_type_e i_type=ITEM_TYPE_PLAYLIST;
    enum input_item_net_type i_net=ITEM_NET_UNKNOWN;
    input_item_t * item= input_item_NewExt (v.data[0],"dummy"/*have to finf a way to extract it*/,t, i_type, i_net);
    //need to test it, if media-type=playlist can be added or not;
    vlc_playlist_InsertOne (playlist,0,item);
    char* inp=vlc_path2uri(v.data[1],"file");
    input_item_t *media=input_item_New(inp,"wit");/*dummy, needs to be extracted*/
    size_t ind=vlc_playlist_Count(playlist);
    vlc_playlist_InsertOne(playlist,ind,media);
    vlc_playlist_Unlock(playlist);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "OK\n");
    msg_Info(intfa, "Save playlist %s.", output);
    destroyVector(&v);
    return (char *)output;
}
char *listplaylists(intf_thread_t *intfa, char *arguments) //same problem of parsing files with particular extension - need to study regex for it
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}

// commands initial
char *commands(intf_thread_t *intfa, char *arguments){}
char *not_commands(intf_thread_t *intfa, char *arguments){}
char *handle_update(intf_thread_t *intfa, char *arguments){}

//other commands
char *urlhandlers(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *decoders(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *kill(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *listfiles(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *lsinfo(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *update(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *rescan(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *setvol(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *volume(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *stats(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *config(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *idle(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}

//output commands
char *enableoutput(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *disableoutput(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *toggleoutput(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *outputset(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *devices(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}

//client connection handling commands
char *closeF(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *ping(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *password(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *tagtypes(intf_thread_t *intfa, char *arguments)
{
    vect v;
    vlc_vector_init(&v);
    getArg(intfa,arguments,&v);
    
    vlc_vector_clear(&v);
    vlc_vector_destroy(&v);
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}

#pragma endregion

// Helper Function Implementation
#pragma region
/* Implementation of Helper Function */
// Reverse Character Array (helper function for itoaFunction)
void reverse(char str[], int length) 
{ 
    int start = 0; 
    int end = length -1; 
    while (start < end) 
    { 
        swap(*(str+start), *(str+end)); 
        start++; 
        end--; 
    } 
} 

// Integer to String Function 
char* itoaFunction(int num, char* str, int base) 
{ 
    // Initialize result 
    int i = 0; 
    bool isNegative = false; 
  
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0) 
    { 
        str[i++] = '0'; 
        str[i] = '\0'; 
        return str; 
    } 
  
    // In standard itoaFunction(), negative numbers are handled only with base 10. Otherwise numbers are considered unsigned. 
    if (num < 0 && base == 10) 
    { 
        isNegative = true; 
        num = -num; 
    } 
  
    // Process individual digits 
    while (num != 0) 
    { 
        int rem = num % base; 
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0'; 
        num = num/base; 
    } 
  
    // If number is negative, append '-' 
    if (isNegative) 
        str[i++] = '-'; 
  
    str[i] = '\0'; // Append string terminator 
  
    // Reverse the string 
    reverse(str, i); 
  
    return str; 
} 

// String to Integer Function
int atoiFunction(char* str) 
{ 
    // Initialize result 
    int res = 0; 
  
    // Iterate through all characters of input string and update result 
    for (int i=0;str[i]!='\0';++i) 
        res = res * 10 + str[i] - '0'; 
  
    // Return result
    return res; 
}

// VLC Player State Change Registerer
static void player_on_state_changed(vlc_player_t *player, enum vlc_player_state state, void *data)
{
    VLC_UNUSED(player);
    char const *psz_cmd;
    switch (state)
    {
        case VLC_PLAYER_STATE_STOPPING: psz_cmd = "stop";
        break;
        case VLC_PLAYER_STATE_STOPPED: psz_cmd = "stop";
        break;
        case VLC_PLAYER_STATE_PLAYING: psz_cmd = "play";
        break;
        case VLC_PLAYER_STATE_PAUSED: psz_cmd = "pause";
        break;
        default: psz_cmd = "";
        break;
    }
    intf_thread_t *p_intf = data;
}

// Compare String (helper function for searchCommand)
static int stringCompare(const void *key, const void *arrayElement)
{
    const commandFunc *commandFunc = arrayElement;
    return strcmp(key, commandFunc->command); /*each list item's string binary search*/
}

// Command Search Function
commandFunc* searchCommand(const char *key)
{
    return bsearch(key, command_list_array, ARRAY_SIZE(command_list_array), sizeof(*command_list_array), stringCompare);
}

// Remove Leading & Trailing Spaces in Command
void trim(char *s)
{
    int i, j;
    //leading space
    for (i = 0; s[i] == ' ' || s[i] == '\t'; i++)
    {
        ;
    }
    for (j = 0; s[i]; i++)
    {
        s[j++] = s[i];
    }
    s[j] = '\0';
    //trailing space
    for (i = 0; s[i] != '\0'; i++)
    {
        if (s[i] != ' ' && s[i] != '\t')
        {
            j = i;
        }
    }
    s[j + 1] = '\0';
}

// Parsing and Processing Input
void parseInput(intf_thread_t *intfa, char *input, char *command, char *argument)
{ 
    // 1. remove whitespace in left and right
    trim(input);
    //msg_Info(intfa, "%s %ld", input, strlen(input));

    // 2. copying argument because strchr returns new pointer
    char *arg = NULL;
    arg = strchr(input, ' ');
    int i = 0;
    while (arg[i] != '\0')
    {
        argument[i] = arg[i];
        i++;
    }
    argument[i] = '\0';

    // 3. separate command and argument using whitespace
    if (argument != NULL)
    {
        //msg_Info(intfa, "argument present");
        int s = strlen(input);
        strncat(command, input, s - strlen(argument));
        trim(argument);
    }
    else
    {
        argument = ""; //empty not null
    }
        
    // 4. check for lowercase - here I am explicity converting to small string
    for (int i = 0; command[i]; i++)
    {
        command[i] = tolower(command[i]);
    }

    // 5. remove " " from arguments if present

    // 6. Function Check
    // msg_Info(intfa, "%p %p %p %p", input, command, argument, arg);
    // msg_Info(intfa, "arg:%s.\nargsize:%ld.\n", arg, strlen(arg));
    // msg_Info(intfa, "command:%s.\ncommandSize:%ld.\narguments:%s.\nargumentsSize:%ld.\n", command, strlen(command), argument, strlen(argument));
}

// Extracting Arguments from Command
void getArg(intf_thread_t *intfa, char* str,vect* v)
{
    // Handle No Arguments Case
    if(strlen(str)==0)
    {
        vlc_vector_clear(v);
        return;
    }
    
    // Remove Multiple Consecutive Whitespaces
    int i, x;
    for(i=x=0; str[i]; ++i)
    {
        if(!isspace(str[i]) || (i > 0 && !isspace(str[i-1])))
        str[x++] = str[i];
    }
    str[x] = '\0';
    
    int cnt=0;
    for(int j=0;j<strlen(str);j++)
    {
        if(isspace(str[j]))
        {
            str[j]=' ';//replace all other whitespaces with single space ->help in tokenisation
            cnt++;
        }
        
    }
    cnt=cnt+1;
    //msg_Info(intfa,"%s.\n",str);
    if(cnt==0)
    {
        vlc_vector_push(v,str);
    }
    else
    {
        i=0;
        char* s=" ";
        char* token = strtok(str,s);
        while( token != NULL )
        {
            //msg_Info(intfa,"%s.\n",token);
            vlc_vector_push(v,token);
            i++;
            token = strtok(NULL,s);
        }
    }
    
    // Function Check
    // msg_Info(intfa,"%d",v.size);/**/
    // for(int i=0;i<v.size;i++)
    // msg_Info(intfa,"%s",v.data[i]);
}

// Clear and Destroy Argument Vector
void destroyVector(vect *v)
{
    vlc_vector_clear(v);
    vlc_vector_destroy(v);
}
#pragma endregion

/* VLC Interface Handling Functions*/

// Client Handling Thread Function
void *clientHandling(void *threadArgument) //Polling + Client Req Handling
{
    #pragma region
    // 0. Function Start
    intf_thread_t *intfa = (intf_thread_t *)threadArgument;
    intf_sys_t *p_sys = intfa->p_sys;
    msg_Info(intfa, "Hello from thread side!");

    // 1. Initialising Client
    int server_fd = intfa->p_sys->server_fd;
    struct sockaddr_in clientAddr;
    int client_fd, clientAddrLen = sizeof(clientAddr);

    fd_set currentSockets, readySockets;
    FD_ZERO(&currentSockets);
    FD_ZERO(&readySockets);
    FD_SET(server_fd, &currentSockets);

    // 2. Listening to clients
    if (listen(server_fd, SOMAXCONN) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    #pragma endregion

    // 3. Event Loop
    bool v = true;
    while (v)
    {
        readySockets = currentSockets;
        if (select(FD_SETSIZE, &readySockets, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &readySockets))
            {
                if (i == server_fd)
                {
                    // 0. server has new client connection
                    bzero((char *)&clientAddr, sizeof(clientAddr));
                    if ((client_fd = accept(server_fd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientAddrLen)) < 0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(client_fd, &currentSockets);

                    // 1. return OK connection established
                    char *output = "OK MPD Version 0.21.25";    /*fixme vlc version here*/
                    send(client_fd, output, strlen(output), 0);
                    msg_Info(intfa, "server read: %d\n", client_fd);
                    //v=false;
                }
                else
                {
                    // 0. client has pending read connection (new message)
                    client_fd = i;
                    msg_Info(intfa, "client_fd read: %d\n", client_fd);

                    // 1. read operation ->read data of unknown length - ask mentors
                    int n;
                    char *input = malloc(sizeof(char) * 4096), *argumentsC = malloc(sizeof(char) * 2048), *command = malloc(sizeof(char) * 1024);
                    bzero(input, 4096); bzero(argumentsC, 2048); bzero(command, 1024); /* fixme: reduce size */
                    if ((n = read(client_fd, input, 4095)) < 0)
                    {
                        perror("read");
                        exit(EXIT_FAILURE);
                    }
                    input[strlen(input) - 1] = '\0';

                    // 2. Handle Size = 0 Input
                    if (strlen(input) == 0)
                    {
                        /* fixme: if continued 0 length input, then needs new approach */
                        commandFunc *command = searchCommand("commands");
                        char *output = command->commandName(intfa, "arg");
                        send(client_fd, output, strlen(output), 0);
                        msg_Info(intfa, "Size0 %s", output);
                        free(output);
                        break;
                    }
                    
                    // 3. Parse Input
                    parseInput(intfa,input,command,argumentsC);
                    
                    // 4. Handle Argument is Missing Quotation
                    if(argumentsC=="\"")
                    {
                        msg_Info(intfa,"Argument is Missing Quotation\n");
                        char* output = "ACK Argument is Missing Quotation";
                        strcat(output,"\n");
                        send(client_fd, output, strlen(output), 0);
                    }

                    // 5. State Change Management
                    vlc_player_t *player = vlc_playlist_GetPlayer(p_sys->vlc_playlist);
                    input_item_t *item = NULL;
                    vlc_player_Lock(player);
                    if( item == NULL )
                    {
                        item = vlc_player_GetCurrentMedia(player);
                        //New input has been registered
                        if( item )
                        {
                            char *psz_uri = input_item_GetURI( item );
                            msg_Info(intfa, "STATUS_CHANGE ( new input: %s )", psz_uri );
                            free( psz_uri );
                        }
                    }
                    if( !vlc_player_IsStarted( player ) )
                    {
                        if (item)
                            item = NULL;
                        p_sys->last_state = VLC_PLAYER_STATE_STOPPED;
                        msg_Info(intfa, "STATUS_CHANGE ( stop state: 0 )" );
                    }
                    if( item != NULL )
                    {
                        enum vlc_player_state state = vlc_player_GetState(player);
                        if (p_sys->last_state != state)
                        {
                            switch (state)
                            {
                                case VLC_PLAYER_STATE_STOPPING:
                                case VLC_PLAYER_STATE_STOPPED:
                                    msg_Info(intfa, "STATUS_CHANGE ( stop state: 5 )");
                                    break;
                                case VLC_PLAYER_STATE_PLAYING:
                                    msg_Info(intfa, "STATUS_CHANGE ( play state: 3 )");
                                    break;
                                case VLC_PLAYER_STATE_PAUSED:
                                    msg_Info(intfa, "STATUS_CHANGE ( pause state: 4 )");
                                    break;
                                default:
                                    break;
                            }
                            p_sys->last_state = state;
                        }
                    }
                    // if( item && b_showpos )
                    // {
                    //     i_newpos = 100 * vlc_player_GetPosition( player );
                    //     if( i_oldpos != i_newpos )
                    //     {
                    //         i_oldpos = i_newpos;
                    //         msg_rc( "pos: %d%%", i_newpos );
                    //     }
                    // }
                    vlc_player_Unlock(player);
                    
                    // 6. search for the command in the array
                    commandFunc* commandF=searchCommand(command);
                    if(commandF==NULL)
                    {
                        msg_Info(intfa,"ACK Command Does Not Exist\n");
                        char* output = "ACK Command Does Not Exist";
                        strcat(output,command);strcat(output,"\n");
                        send(client_fd, output, strlen(output), 0);
                        //FD_CLR(client_fd, &currentSockets);
                    }
                    else
                    {
                        msg_Info(intfa,"ACK Command Does Exist\n");
                        char* output = commandF->commandName(intfa, argumentsC);
                        if(!strcmp(output,"Invalid Argument"))
                        {
                            msg_Info(intfa,"Invalid Argument\n");
                            char* outputArg = "ACK Invalid Argument ";
                            strcat(outputArg,argumentsC);strcat(outputArg,"\n");
                            send(client_fd, outputArg, strlen(outputArg), 0);
                        }
                        else
                        {
                            msg_Info(intfa,"Valid Argument\n");
                            send(client_fd, output, strlen(output), 0);    
                        }
                        free(output);
                    }

                    // 7. free up dynamically allocated data
                    free(input);    free(command);  free(argumentsC);
                    //FD_CLR(client_fd, &currentSockets); //remove client_fd to current
                }
            }
        }//for
    }//while

    // 4. Function End
    return NULL;
}

// Open Module
static int Open(vlc_object_t *obj)
{
    // 0. Module Start
    intf_thread_t *intfa = (intf_thread_t *)obj;
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
    int opt = 1;
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
    intf_sys_t *p_sys = malloc(sizeof(*p_sys));
    if (unlikely(p_sys == NULL))
        return VLC_ENOMEM;

    // 6. VLC MediaLibrary, Playlist, Player, Listner Initialisation
    intf_thread_t *intf = (intf_thread_t *)obj;
    intf->p_sys = p_sys;
    p_sys->server_fd = server_fd;
    p_sys->vlc_medialibrary = vlc_ml_instance_get(obj);
    p_sys->vlc_playlist = vlc_intf_GetMainPlaylist(intf);
    //p_sys->vlc_player = vlc_playlist_GetPlayer(p_sys->vlc_playlist);
    vlc_player_t *player = vlc_playlist_GetPlayer(p_sys->vlc_playlist);

    // 7. Client Handling
    int i = 0;
    if ((i = vlc_clone(&intf->p_sys->thread, clientHandling, intf, VLC_THREAD_PRIORITY_LOW)) < 0)
    {
        perror("clone failed");
        exit(EXIT_FAILURE);
    }
    
    // 8. Attaching Listners to Player
    static struct vlc_player_cbs const player_cbs =
        {
            .on_state_changed = player_on_state_changed
            // , 
            // .on_buffering_changed = player_on_buffering_changed,
            // .on_rate_changed = player_on_rate_changed,
            // .on_position_changed = player_on_position_changed,
        };
    vlc_player_Lock(player);
    p_sys->player_listener = vlc_player_AddListener(player, &player_cbs, intf);
    if (!p_sys->player_listener)
    {
        vlc_player_Unlock(player);
        goto error;
    }
    vlc_player_Unlock(player);
    // static struct vlc_player_aout_cbs const player_aout_cbs =
    // {
    //     .on_volume_changed = player_aout_on_volume_changed,
    // };
    // p_sys->player_aout_listener =
    //     vlc_player_aout_AddListener(player, &player_aout_cbs, p_intf);
    // vlc_player_Unlock(player);
    // if (!p_sys->player_aout_listener)
    //     goto error;

    // 9. Module End
    return VLC_SUCCESS;
error:
    if (p_sys->player_listener)
    {
        vlc_player_Lock(player);
        vlc_player_RemoveListener(player, p_sys->player_listener);
        vlc_player_Unlock(player);
    }
    free(p_sys);
    return VLC_EGENERIC;
}

// Close Module
static void Close(vlc_object_t *obj)
{
    // 0. Module Start
    intf_thread_t *intf = (intf_thread_t *)obj;

    // 1. Freeing up resources
    // -those dynamically allocated, not static allocation (like int i = 10;)
    // check for appropriate dynamic allocation, I have used only static allocation
    intf_sys_t *p_sys = intf->p_sys;
    vlc_cancel(intf->p_sys->thread);
    vlc_join(intf->p_sys->thread, NULL);
    vlc_player_t *player = vlc_playlist_GetPlayer(p_sys->vlc_playlist);
    vlc_player_Lock(player);
    vlc_player_RemoveListener(player, p_sys->player_listener);
    //vlc_player_aout_RemoveListener(player, p_sys->player_aout_listener);
    vlc_player_Unlock(player);

    // free(sys->vlc_medialibrary);
    // free(sys->vlc_playlist);
    // free(sys->vlc_player);
    free(p_sys);

    // 2. Module End
    msg_Info(intf, "Good bye MPD Server");
}

#pragma region
// Module Descriptor
vlc_module_begin()
    set_shortname(N_("MPD"))
    set_description(N_("MPD Control Interface Module"))
    set_capability("interface", 0)
    set_callbacks(Open, Close)
    set_category(CAT_INTERFACE)
vlc_module_end()

/*
msg_Info(intfa,"1");
vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
msg_Info(intfa,"2");
vlc_player_t *player=vlc_playlist_GetPlayer(playlist);
msg_Info(intfa,"3");
char* inp=vlc_path2uri("/home/nightwayne/Music/IDWItTakes.mp3","file");
input_item_t *media=input_item_New(inp,"wit"); //URI file path error
msg_Info(intfa,"4");
//playing1
if(player==NULL)
msg_Info(intfa,"NN");
if(playlist==NULL)
msg_Info(intfa,"NNds");
int t;
vlc_player_Lock(player);
msg_Info(intfa,"5");
t=vlc_player_SetCurrentMedia(player, media);
msg_Info(intfa,"6 %d",t);
t=vlc_player_Start(player);
msg_Info(intfa,"7 %d",t);
sleep(2);
msg_Info(intfa,"8");
vlc_player_Stop(player);
msg_Info(intfa,"9");
vlc_player_Unlock(player);
msg_Info(intfa,"10");
input_item_Release (media);
msg_Info(intfa,"11");
vlc_player_Delete(player);
msg_Info(intfa,"12");
vlc_playlist_Delete(playlist);
msg_Info(intfa,"13");
*/
/*
static void PlaylistDoVoid(intf_thread_t *intf, int (*cb)(vlc_playlist_t *))
{
    vlc_playlist_t *playlist = intf->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    msg_Info(intf,"playlistcbslock");
    char* inp=vlc_path2uri("/home/nightwayne/Music/IDWItTakes.mp3","file");
    input_item_t *media=input_item_New(inp,"wit"); //URI file path error
    int i = vlc_playlist_InsertOne(playlist, 0, media);
    msg_Info(intf,"playlistcbslockcall");
    cb(playlist);
    msg_Info(intf,"playlistcbsbyelock");
    vlc_playlist_Unlock(playlist);
}
static void PlaylistPlay(intf_thread_t *intf)
{
    msg_Info(intf,"play");
    PlaylistDoVoid(intf, vlc_playlist_Start);
}
static int PlaylistDoStop(vlc_playlist_t *playlist)
{
    vlc_playlist_Stop(playlist);
    return 0;
}
static void PlaylistStop(intf_thread_t *intf)
{
    PlaylistDoVoid(intf, PlaylistDoStop);
}
*/
/*
    //playback testing
    // vlc_playlist_Lock(intfa->p_sys->vlc_playlist);
    // char* inp=vlc_path2uri("/home/nightwayne/Music/IDWItTakes.mp3","file");
    // input_item_t *media=input_item_New(inp,"wit"); //URI file path error
    // size_t ind=vlc_playlist_Count(intfa->p_sys->vlc_playlist);
    // vlc_playlist_InsertOne(intfa->p_sys->vlc_playlist,ind,media);
    // media=input_item_New("file:///home/nightwayne/Music/Natural.mp3","Nat");
    // ind=vlc_playlist_Count(intfa->p_sys->vlc_playlist);
    // vlc_playlist_InsertOne(intfa->p_sys->vlc_playlist,ind,media);
    // vlc_playlist_Unlock(intfa->p_sys->vlc_playlist);
    // playTest(intfa,media);
    // sleep(7);
    // pauseTest(intfa);
*/
#pragma endregion