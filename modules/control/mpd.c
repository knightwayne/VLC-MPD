#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
/* Header Files */
#pragma region
/*Standard Header Files*/
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <stdint.h>
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
#include <vlc_input_item.h>
#include <vlc_url.h>
#include <vlc_playlist.h>
#include <vlc_playlist_export.h>
#include <vlc_player.h>
#include <vlc_media_library.h>
#include <vlc_mpd.h>
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

    /* status changes */
    vlc_mutex_t status_lock;
    enum vlc_player_state last_state;
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

//MPD Command Helper Function: these functions don't directly respond to client inputs, they help the client-command handling functions
void addRecursively(intf_thread_t *intfa, char *basePath); //adding files from folders
void playlistInfo(intf_thread_t *intfa, vlc_playlist_t *playlist, char* output, FILE *f, char* path, int i); //metadata of songs in playlist
void databaseLookup(intf_thread_t *intfa, char* type, vlc_ml_query_params_t* params, FILE *f, char* path, bool addToPlaylist); //search helper tool, using media library query_params data structure
void listRecursively(intf_thread_t *intfa, char *basePath, FILE *f, char* filePath, bool metadata); //listing files/folders in mpd database

/* Command Array */
const commandFunc command_list_array[] = {
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
    {"update", update},
    {"urlhandlers", urlhandlers},
    {"volume", volume}
};
#pragma endregion

// MPD Interaction Commands Available
#pragma region
//1. Query MPD Status
char *clearerror(intf_thread_t *intfa, char *arguments)
{
    char *output=strdup("OK\n");
    msg_Info(intfa, "Clear Error %s.", output);
    return (char *)output;
}
char *currentsong(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char* output;
    
    char *fileWrite = malloc(sizeof(char) * 64);bzero(fileWrite, 64);
    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt");

    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);

    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    input_item_t *item = vlc_player_GetCurrentMedia(player);
    int i = vlc_playlist_IndexOfMedia(playlist,item);
    playlistInfo(intfa,playlist,fileWrite,f,path,i);
    vlc_playlist_Unlock(playlist);

    strcat(fileWrite, "OK\n"); f = fopen (path,"a+"); fputs(fileWrite,f); fclose(f);
    output=strdup("file"); //denoting file mode of output;
    free(fileWrite); free(path);

    destroyVector(&v); return (char *)output;
}
char *idle(intf_thread_t *intfa, char *arguments)
{
    //handled in Client Handling Function, as a special case
    //redundant function case
}
/*fixme: status and stats, can't extract information, input_item_t doesn't grab value as it should*/
char *status(intf_thread_t *intfa, char *arguments) //need to look at all state changes and report then
{
    // Unable to extract all information through playlists and player
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *stats(intf_thread_t *intfa, char *arguments)
{
    // Unable to extract all information through playlists and player
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}

//9. Connection Settings
char *closeF(intf_thread_t *intfa, char *arguments)
{
    //handled in Client Handling Function, as a special case
    //redundant function case
}
char *kill(intf_thread_t *intfa, char *arguments)
{
    // Advised by mentors not to implement Killing MPD Server through Client. Will close VLC also.
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *password(intf_thread_t *intfa, char *arguments)
{
    // Password Verification Not Implemented
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *ping(intf_thread_t *intfa, char *arguments)
{
    char* output=strdup("OK\n");
    return (char *)output;
}
char *tagtypes(intf_thread_t *intfa, char *arguments)
{
    //currently tags limited by medialibrary
    char* output=strdup("Album\nArtist\nGenre\nOK\n");
    return (char *)output;
}
#pragma endregion

// Playback Options & Settings
#pragma region
//2. Playback Options
char *consume(intf_thread_t *intfa, char *arguments) 
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;

    int state=atoi(v.data[0]);
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    if(state==1)
    {
        enum vlc_playlist_playback_repeat mode=VLC_PLAYLIST_PLAYBACK_REPEAT_NONE;
        vlc_playlist_SetPlaybackRepeat (playlist, mode);
        
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else if(state==0)
    {
        enum vlc_playlist_playback_repeat mode=VLC_PLAYLIST_PLAYBACK_REPEAT_ALL;
        vlc_playlist_SetPlaybackRepeat (playlist, mode);
        
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else
    {
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    vlc_playlist_Unlock(playlist);
    
    destroyVector(&v); return (char *)output;
}
char *repeat(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;

    int state=atoi(v.data[0]);
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    if(state==1)
    {
        enum vlc_playlist_playback_repeat mode=VLC_PLAYLIST_PLAYBACK_REPEAT_CURRENT;
        vlc_playlist_SetPlaybackRepeat (playlist, mode);
        
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else if(state==0)
    {
        enum vlc_playlist_playback_repeat mode=VLC_PLAYLIST_PLAYBACK_REPEAT_ALL;
        vlc_playlist_SetPlaybackRepeat (playlist, mode);
        
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else
    {
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    vlc_playlist_Unlock(playlist);
    
    destroyVector(&v); return (char *)output;
}
char *randomF(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;

    int state=atoi(v.data[0]);
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    if(state==1)
    {
        enum vlc_playlist_playback_order mode=VLC_PLAYLIST_PLAYBACK_ORDER_RANDOM;
        vlc_playlist_SetPlaybackOrder (playlist, mode);
        
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else if(state==0)
    {
        enum vlc_playlist_playback_order mode=VLC_PLAYLIST_PLAYBACK_ORDER_NORMAL;
        vlc_playlist_SetPlaybackOrder (playlist, mode);
        
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else
    {
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    vlc_playlist_Unlock(playlist);
    
    destroyVector(&v); return (char *)output;
}
char *single(intf_thread_t *intfa, char *arguments) //how to stop playback after completing song
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;

    int state=atoi(v.data[0]);
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_playlist_Lock(playlist);
    if(state==1)
    {
        enum vlc_playlist_playback_repeat mode=vlc_playlist_GetPlaybackRepeat(playlist);
        if(mode==VLC_PLAYLIST_PLAYBACK_REPEAT_CURRENT)
        {
            mode=VLC_PLAYLIST_PLAYBACK_REPEAT_CURRENT;
            vlc_playlist_SetPlaybackRepeat (playlist, mode);
        }
        else
        {
            vlc_player_Stop(player);
        }

        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else if(state==0) //normal playback -
    {
        enum vlc_playlist_playback_repeat mode=VLC_PLAYLIST_PLAYBACK_REPEAT_ALL;
        vlc_playlist_SetPlaybackRepeat (playlist, mode);
        
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else
    {
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    vlc_playlist_Unlock(playlist);
    
    destroyVector(&v); return (char *)output;
}
char *setvol(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;

    int volume=atoi(v.data[0]);
    float fVolume=(float)volume/100.0;
    msg_Info(intfa,"%d %f",volume,fVolume);
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_playlist_Lock(playlist);
    vlc_player_aout_SetVolume(player,fVolume);
    vlc_playlist_Unlock(playlist);
    
    output=strdup("OK\n");
    //msg_Info(intfa, "Clear %s.", output);
    
    destroyVector(&v); return (char *)output;
}
char *volume(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;

    int volume=atoi(v.data[0]);
    float fVolume=(float)volume/100.0;
    msg_Info(intfa,"%d %f",volume,fVolume);
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_playlist_Lock(playlist);
    vlc_player_aout_SetVolume(player,fVolume);
    vlc_playlist_Unlock(playlist);
    
    output=strdup("OK\n");
    //msg_Info(intfa, "Clear %s.", output);
    
    destroyVector(&v); return (char *)output;
}
// Features Not Available in VLC
char *crossfade(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *mixrampdb(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *mixrampdelay(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *replay_gain_mode(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *replay_gain_status(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}

//3. Controlling Playback
char *play(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    int pos=atoi(v.data[0]);

    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_playlist_Lock(playlist);
    int size=vlc_playlist_Count(playlist);
    if((pos<size)&&(pos>-1)) //Valid Positions
    {
        vlc_playlist_item_t *playlistItem= vlc_playlist_Get(playlist,pos);
        input_item_t *item=vlc_playlist_item_GetMedia (playlistItem);
        vlc_player_SetCurrentMedia(player, item);
        vlc_player_Start(player);
        
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else //Invalid Positions
    {
        output=strdup("Error: Bad Position/Index.");
        //msg_Info(intfa,"Error: Bad Position/Index.");
    }
    vlc_playlist_Unlock(playlist);

    destroyVector(&v); return (char *)output;
}
char *playid(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    int id=atoi(v.data[0]);

    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_playlist_Lock(playlist);
    int pos=vlc_playlist_IndexOfId(playlist, id);
    if(pos!=-1) //ID Exists
    {
        vlc_playlist_item_t *playlistItem= vlc_playlist_Get(playlist,pos);
        input_item_t *item=vlc_playlist_item_GetMedia (playlistItem);
        vlc_player_SetCurrentMedia(player, item);
        vlc_player_Start(player);
        
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else //Invalid Positions
    {
        output=strdup("Error: Bad Position/Index.");
        //msg_Info(intfa,"Error: Bad Position/Index.");
    }
    vlc_playlist_Unlock(playlist);

    destroyVector(&v); return (char *)output;
}
char *pauseF(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    int pause=atoi(v.data[0]);

    vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_playlist_Lock(playlist);

    if(pause==1) //ID Exists
    {
        vlc_player_Pause(player);
        
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else if(pause==0)//Invalid Positions
    {
        vlc_player_Resume(player);
        
        output=strdup("OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else //Invalid Positions
    {
        output=strdup("Error: Invalid Argument.");
        //msg_Info(intfa,"Error: Bad Position/Index.");
    }
    
    vlc_playlist_Unlock(playlist);

    destroyVector(&v); return (char *)output;
}
char *stop(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;

    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_playlist_Lock(playlist);
    vlc_player_Stop(player);
    vlc_playlist_Unlock(playlist);
    
    output=strdup("OK\n");
    //msg_Info(intfa, "Clear %s.", output);
    
    destroyVector(&v); return (char *)output;
}
char *next(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;

    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_playlist_Lock(playlist);

    if(!vlc_playlist_HasNext(playlist))
    {
        output=strdup("Error:End of playlist\n");
        //msg_Info(intfa, "Clear %s.", output);
    }
    else
    {
        vlc_playlist_Next(playlist);
        int pos=vlc_playlist_GetCurrentIndex(playlist);
        vlc_playlist_item_t *playlistItem= vlc_playlist_Get(playlist,pos);
        input_item_t *item=vlc_playlist_item_GetMedia (playlistItem);
        
        if(vlc_player_IsStarted(player))
        vlc_player_Stop(player);
        
        vlc_player_SetCurrentMedia(player, item);
        vlc_player_Start(player);   
        
        output=strdup("OK\n");
        //msg_Info(intfa, "Clear %s.", output); 
    }
    vlc_playlist_Unlock(playlist);
    
    destroyVector(&v); return (char *)output;
}
char *previous(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;

    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_playlist_Lock(playlist);

    if(!vlc_playlist_HasPrev(playlist))
    {
        output=strdup("Error:Start of playlist\n");
        //msg_Info(intfa, "Clear %s.", output);
    }
    else
    {
        vlc_playlist_Prev(playlist);
        int pos=vlc_playlist_GetCurrentIndex(playlist);
        vlc_playlist_item_t *playlistItem= vlc_playlist_Get(playlist,pos);
        input_item_t *item=vlc_playlist_item_GetMedia (playlistItem);
        
        if(vlc_player_IsStarted(player))
        vlc_player_Stop(player);
        
        vlc_player_SetCurrentMedia(player, item);
        vlc_player_Start(player);   
        
        output=strdup("OK\n");
        //msg_Info(intfa, "Clear %s.", output); 
    }
    vlc_playlist_Unlock(playlist);
    
    destroyVector(&v); return (char *)output;
}
char *seekcur(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;

    int t; bool relative=false;
    if(v.data[0][0]=='+')
    {
        char* str=v.data[0];str++;
        t=atoi(str);
        relative=true;
    }
    else if(v.data[0][0]=='-')
    {
        char* str=v.data[0];str++;
        t=atoi(str); t=t*(-1);
        relative=true;
    }
    else
    {
        t=atoi(v.data[0]);
    }
    //msg_Info(intfa,"%d",t);

    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_playlist_Lock(playlist);
    if(relative)
    {
        vlc_tick_t curr=VLC_TICK_FROM_SEC(t);
        enum vlc_player_seek_speed speed=VLC_PLAYER_SEEK_FAST;
        enum vlc_player_whence whence=VLC_PLAYER_WHENCE_RELATIVE;
        if(vlc_player_IsStarted(player))
        vlc_player_Pause(player);
        vlc_player_SeekByTime (player,curr,speed,whence);
        vlc_player_Resume(player);
    }
    else
    {
        vlc_tick_t curr=VLC_TICK_FROM_SEC(t);
        enum vlc_player_seek_speed speed=VLC_PLAYER_SEEK_FAST;
        enum vlc_player_whence whence=VLC_PLAYER_WHENCE_ABSOLUTE;
        if(vlc_player_IsStarted(player))
        vlc_player_Pause(player);
        vlc_player_SeekByTime (player,curr,speed,whence);
        vlc_player_Resume(player);
    }
    vlc_playlist_Unlock(playlist);

    output=strdup("OK\n");
    //msg_Info(intfa, "Clear %s.", output);
    destroyVector(&v); return (char *)output;
}
// code-wise, both functions are implemented, but will not work in VLC, because can't seek a non-playing media in VLC. Similair to rangeid command.
char *seek(intf_thread_t *intfa, char *arguments)//cant seek 
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output = malloc(sizeof(char) * 512); bzero(output, 512);

    int pos=atoi(v.data[0]); int t=atoi(v.data[1]);

    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_playlist_Lock(playlist);
    int size=vlc_playlist_Count(playlist);
    if((pos<size)&&(pos>-1)) //Valid Positions
    {
        vlc_playlist_item_t *playlistItem= vlc_playlist_Get(playlist,pos);
        input_item_t *item=vlc_playlist_item_GetMedia (playlistItem);
        vlc_tick_t curr=VLC_TICK_FROM_SEC(t);
        // int z=SEC_FROM_VLC_TICK(curr); msg_Info(intfa,"%d",z);
        enum vlc_player_seek_speed speed=VLC_PLAYER_SEEK_FAST;
        enum vlc_player_whence whence=VLC_PLAYER_WHENCE_ABSOLUTE;
        // if(vlc_player_IsStarted(player))
        // vlc_player_Stop(player);

        vlc_player_SetCurrentMedia(player, item);
        vlc_player_SeekByTime(player,curr,speed,whence);
        vlc_player_Start(player);

        strcat(output,"OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else //Invalid Positions
    {
        strcat(output,"Error: Bad Position/Index.");
        //msg_Info(intfa,"Error: Bad Position/Index.");
    }
    vlc_playlist_Unlock(playlist);

    destroyVector(&v); return (char *)output;
}
char *seekid(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output = malloc(sizeof(char) * 512); bzero(output, 512);

    int id=atoi(v.data[0]); int t=atoi(v.data[1]);

    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    vlc_playlist_Lock(playlist);
    int pos=vlc_playlist_IndexOfId(playlist, id);
    int size=vlc_playlist_Count(playlist);
    if(pos!=-1) //Valid Positions
    {
        vlc_playlist_item_t *playlistItem= vlc_playlist_Get(playlist,pos);
        input_item_t *item=vlc_playlist_item_GetMedia (playlistItem);
        vlc_tick_t curr=VLC_TICK_FROM_SEC(t);
        // int z=SEC_FROM_VLC_TICK(curr); msg_Info(intfa,"%d",z);
        enum vlc_player_seek_speed speed=VLC_PLAYER_SEEK_FAST;
        enum vlc_player_whence whence=VLC_PLAYER_WHENCE_ABSOLUTE;
        // if(vlc_player_IsStarted(player))
        // vlc_player_Stop(player);

        vlc_player_SetCurrentMedia(player, item);
        vlc_player_SeekByTime(player,curr,speed,whence);
        vlc_player_Start(player);

        strcat(output,"OK\n");
        //msg_Info(intfa,"Playling at Pos: %d.",pos);
    }
    else //Invalid Positions
    {
        strcat(output,"Error: Bad Position/Index.");
        //msg_Info(intfa,"Error: Bad Position/Index.");
    }
    vlc_playlist_Unlock(playlist);

    destroyVector(&v); return (char *)output;
}
#pragma endregion

// Queue (Current Playlist Management)
#pragma region
//4. Queue
void addRecursively(intf_thread_t *intfa, char *basePath)
{
    char path[1000]; struct dirent *dp; DIR *dir = opendir(basePath);

    if (!dir)
    return;
    else
    {
        while ((dp = readdir(dir)) != NULL)
        {
            if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
            {
                strcpy(path, basePath); strcat(path, "/"); strcat(path, dp->d_name);
                struct stat st_buf; int status = stat (path, &st_buf);
                
                if (status==0 && S_ISREG (st_buf.st_mode)) //Adding File
                {
                    char* uri=vlc_path2uri(path,"file");
                    char* name=strrchr(uri,'/'); name++;
                    input_item_t *media=input_item_New(uri,name);
                    
                    if(!media) //Media Not Converted to Input Item
                    {
                        //should never reach here
                        //msg_Info(intfa, "Error: Converting Media into Input Item.");
                    }
                    else //Media Converted to Input Item
                    {
                        vlc_playlist_t *playlist = intfa->p_sys->vlc_playlist;
                        vlc_playlist_Lock(playlist);
                        vlc_playlist_InsertOne(playlist, vlc_playlist_Count(playlist), media);
                        vlc_playlist_Unlock(playlist);

                        //msg_Info(intfa, "Added File: %s.", name);
                    }
                    input_item_Release(media); 
                }
                else //Not A File
                {
                    //should never reach here
                    //msg_Info(intfa, "Error: Path Not a File.");
                }
                        
                addRecursively(intfa, path);
            }
        }
    }
    closedir(dir);
}
char *add(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    char* uri=v.data[0]; char* path = vlc_uri2path(uri);
    struct stat st_buf; int status = stat (path, &st_buf);

    if (status==0 && S_ISDIR (st_buf.st_mode)) //Adding Folder Recursively
    {
        addRecursively(intfa, path);
        output=strdup("OK\n");
        //msg_Info(intfa, "Added Folder: %s.", path);
    }
    else if (status==0 && S_ISREG (st_buf.st_mode)) //Adding File
    {
        char* name=strrchr(uri,'/'); name++;
        input_item_t *media=input_item_New(uri,name);
        
        if(!media) //Media Not Converted to Input Item
        {
            output=strdup("Error: Converting Media into Input Item.");
            //msg_Info(intfa, "Error: Converting Media into Input Item.");
        }
        else //Media Converted to Input Item
        {
            vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
            vlc_playlist_Lock(playlist);
            vlc_playlist_InsertOne(playlist,vlc_playlist_Count(playlist),media);
            vlc_playlist_Unlock(playlist);    
            
            output=strdup("OK\n");
            //msg_Info(intfa, "Added File %s.", name);
        }
        input_item_Release(media); 
    }
    else //Not A File
    {
        output=strdup("Error: Path URI Not Valid.");
        //msg_Info(intfa,"Error: Path URI Not Valid.");
    }
    
    destroyVector(&v); return (char *)output;
}
char *addid(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output = malloc(sizeof(char) * 512);bzero(output, 512);/*not using strdup here, because the buffer needs to be concatenated, not a fixed string*/

    char* uri=v.data[0]; char* path = vlc_uri2path(uri);
    struct stat st_buf; int status = stat (path, &st_buf);
    
    if (status==0 && S_ISREG (st_buf.st_mode)) //Adding File
    {
        char* name=strrchr(uri,'/'); name++;
        input_item_t *media=input_item_New(uri,name);
        
        if(!media) //Media Not Converted to Input Item
        {
            strcat(output,"Error: Converting Media into Input Item.");
            //msg_Info(intfa, "Error: Converting Media into Input Item.");
        }
        else if(v.size==1) //Argument=[URI]
        {
            vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
            vlc_playlist_Lock(playlist);
            int size=vlc_playlist_Count(playlist);
            vlc_playlist_InsertOne(playlist,size,media);
            vlc_playlist_item_t *item = vlc_playlist_Get(playlist,size);
            int id=vlc_playlist_item_GetId(item);
            char *idString[4]; sprintf(idString, "%d", id); 
            //vlc_playlist_item_Release(item);
            vlc_playlist_Unlock(playlist);
            
            strcat(output,"ID: "); strcat(output,idString); strcat(output, "\nOK\n");
            //msg_Info(intfa, "Added(id) File %s.", name);
        }
        else if(v.size==2) //Argument=[URI POS]
        {
            int pos=atoi(v.data[1]);
            
            vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
            vlc_playlist_Lock(playlist);
            int size=vlc_playlist_Count(playlist);
            if((pos<size)&&(pos>-1)) //Valid Positions
            {
                vlc_playlist_InsertOne(playlist,pos,media);
                vlc_playlist_item_t *item = vlc_playlist_Get(playlist,pos);
                int id=vlc_playlist_item_GetId(item);
                char *idString[4]; sprintf(idString, "%d", id); 
                //vlc_playlist_item_Release(item);

                strcat(output,"ID: "); strcat(output,idString); strcat(output, "\nOK\n");
                //msg_Info(intfa, "Added(id) File %s.", name);
            }
            else //Invalid Positions
            {
                strcat(output,"Error: Bad Position/Index.");
                //msg_Info(intfa,"Error: Bad Position/Index.");
            }
            vlc_playlist_Unlock(playlist);
        }
        else //Invalid Number of Arguments
        {
            strcat(output,"Error: Invalid Type/Number of Arguments.");
            //msg_Info(intfa,Error: Invalid Type/Number of Arguments.");
        }
        input_item_Release(media); 
    }
    else //Not a File
    {
        strcat(output,"Error: Path URI Not Valid.");
        //msg_Info(intfa,"Error: Path URI Not Valid.");
    }

    destroyVector(&v); return (char *)output;
}
char *clear(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;

    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    vlc_playlist_Clear(playlist);
    vlc_playlist_Unlock(playlist);
    
    output=strdup("OK\n");
    //msg_Info(intfa, "Clear %s.", output);
    
    destroyVector(&v); return (char *)output;
}
char *deleteF(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    char *ifColonPresent = strchr(v.data[0],':');
    
    if(ifColonPresent!=NULL) //Argument=[Start:End]
    {
        int start=0,end=0,ind=0;
        //checking argument validity 
        while(v.data[0][ind]!='\0')
        {
            if( ((int)(v.data[0][ind])>=48) && ((int)(v.data[0][ind])<=58)) //either digit or : symbol(ascii=58) 
            ind++;
            else //not valid argument
            {
                output=strdup("Error: Invalid Type/Number of Arguments.");
                destroyVector(&v); return (char *)output;
            }
        }
        //separating before and after :
        ind=0;
        while(v.data[0][ind]!=':')
        {ind++;}
        //extracting numbers from arguments
        char* beforeColon[5]; strncpy(beforeColon,v.data[0],ind); start=atoi(beforeColon);
        ifColonPresent++; end=atoi(ifColonPresent);
        
        //deleting from queue
        vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
        vlc_playlist_Lock(playlist);
        int size=vlc_playlist_Count(playlist);
        if((start>-1)&&(end<=size)&&(start<end)) //Valid Positions
        {
            vlc_playlist_Remove(playlist,start,end-start);
            
            output=strdup("OK\n");
            //msg_Info(intfa,"Deleted from Start: %d to End: %d.",start,end);
        }
        else //Invalid Positions
        {
            output=strdup("Error: Bad Position/Index.");
            //msg_Info(intfa,"Error: Bad Position/Index.");
        }
        vlc_playlist_Unlock(playlist);
    }
    else if(ifColonPresent==NULL) //Argument=[Pos]
    {
        int pos=atoi(v.data[0]);

        vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
        vlc_playlist_Lock(playlist);
        int size=vlc_playlist_Count(playlist);
        if((pos<size)&&(pos>-1)) //Valid Positions
        {
            vlc_playlist_RemoveOne(playlist,pos);
            
            output=strdup("OK\n");
            //msg_Info(intfa,"Deleted at Pos: %d.",pos);
        }
        else //Invalid Positions
        {
            output=strdup("Error: Bad Position/Index.");
            //msg_Info(intfa,"Error: Bad Position/Index.");
        }
        vlc_playlist_Unlock(playlist);
    }
    
    destroyVector(&v); return (char *)output;
}
char *deleteid(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    int id=atoi(v.data[0]);

    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    int pos=vlc_playlist_IndexOfId(playlist, id);
    if(pos!=-1) //ID Exists
    {
        vlc_playlist_RemoveOne (playlist,pos);
       
        output=strdup("OK\n");
        //msg_Info(intfa, "Deleted Id: %d.",pos);
    }
    else // ID Doesn't Exists
    {
        output=strdup("Error: Not Valid ID");
        //msg_Info(intfa,"Error: Not Valid ID");
    }
    vlc_playlist_Unlock(playlist);
    
    destroyVector(&v); return (char *)output;
}
char *move(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    char *ifColonPresent = strchr(v.data[0],':');
    
    if(ifColonPresent!=NULL) //Argument=[Start:End]
    {
        int start=0,end=0,ind=0;
        //checking argument validity 
        while(v.data[0][ind]!='\0')
        {
            if( ((int)(v.data[0][ind])>=48) && ((int)(v.data[0][ind])<=58)) //either digit or : symbol(ascii=58) 
            ind++;
            else //not valid argument
            {
                output=strdup("Error: Invalid Type/Number of Arguments.");
                destroyVector(&v); return (char *)output;
            }
        }
        //separating before and after :
        ind=0;
        while(v.data[0][ind]!=':')
        {ind++;}
        //extracting numbers from arguments
        char* beforeColon[5]; strncpy(beforeColon,v.data[0],ind); start=atoi(beforeColon);
        ifColonPresent++; end=atoi(ifColonPresent);
        int pos_to=atoi(v.data[1]);
        
        vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
        vlc_playlist_Lock(playlist);
        int size=vlc_playlist_Count(playlist);
        if((start>-1)&&(end<=size)&&(start<end)) //Valid Position
        {
            vlc_playlist_Move(playlist,start,end-start,pos_to);
            
            output=strdup("OK\n");
            //msg_Info(intfa,"Moved from Start: %d to End: %d at Pos: %d.",start,end,pos_to);
        }
        else //Invalid Position
        {
            output=strdup("Error: Bad Position/Index.");
            //msg_Info(intfa,"Error: Bad Position/Index.");
        }
        vlc_playlist_Unlock(playlist);
    }
    else if(ifColonPresent==NULL) //Argument=[Pos]
    {
        int pos_from=atoi(v.data[0]);int pos_to=atoi(v.data[1]);
        
        vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
        vlc_playlist_Lock(playlist);
        int size=vlc_playlist_Count(playlist);
        if((pos_from<size)&&(pos_to<size)&&(pos_from>-1)&&(pos_to>-1)) //Valid Position
        {
            vlc_playlist_MoveOne(playlist,pos_from,pos_to);
            
            output=strdup("OK\n");
            //msg_Info(intfa,"Moved from %d to %d.",pos_from,pos_to);
        }
        else //Invalid Position
        {
            output=strdup("Error: Bad Position/Index.");
            //msg_Info(intfa,"Error: Bad Position/Index.");
        }
        vlc_playlist_Unlock(playlist);
    }
    
    destroyVector(&v); return (char *)output;
}
char *moveid(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    int id=atoi(v.data[0]); int pos_to=atoi(v.data[1]);
    
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    int pos_from=vlc_playlist_IndexOfId(playlist, id);
    if(pos_from!=-1) //ID Exists
    {
        vlc_playlist_MoveOne (playlist,pos_from,pos_to);
        
        output=strdup("OK\n");
        //msg_Info(intfa, "Moved Id: %d, from %d to %d.",id,pos_from,pos_to);
    }
    else // ID Doesn't Exists
    {
        output=strdup("Error: Not Valid ID");
        //msg_Info(intfa,"Error: Not Valid ID");
    }
    vlc_playlist_Unlock(playlist);
    
    destroyVector(&v); return (char *)output;

}

char *playlist(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char* output;
    
    FILE *f;
    char *fileWrite = malloc(sizeof(char) * 64);bzero(fileWrite, 64);
    char *path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt");

    char *idString[4];
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    for(int pos=0;pos<vlc_playlist_Count(playlist);pos++)
    {
        //write to file
        vlc_playlist_item_t* playlist_item=vlc_playlist_Get(playlist,pos);
        input_item_t* item= vlc_playlist_item_GetMedia(playlist_item);
        sprintf(idString, "%d", pos); strcat(fileWrite,idString); strcat(fileWrite,":file: "); 
        strcat(fileWrite,item->psz_name); strcat(fileWrite,"\n");
        f = fopen (path,"a+"); fputs(fileWrite,f); fclose(f);/*msg_Info(intfa,"%s",output);*/
        
        //free resources
        input_item_Release(item);
        bzero(fileWrite,strlen(fileWrite)); bzero(idString,strlen(idString));
    }
    vlc_playlist_Unlock(playlist);
    strcat(fileWrite, "OK\n"); f = fopen (path,"a+"); msg_Info(intfa,"fsd"); fputs(fileWrite,f); msg_Info(intfa,"fsdds");  fclose(f);
    free(fileWrite); msg_Info(intfa,"fsdgg"); free(path); msg_Info(intfa,"fsdaaaaads"); 
    
    output=strdup("file"); //denoting file mode of output;
    destroyVector(&v); return (char *)output;
}
void playlistInfo(intf_thread_t *intfa, vlc_playlist_t *playlist, char* output, FILE *f, char* path, int i)
{
    char *intString[4]; 
    vlc_playlist_item_t* playlistItem; input_item_t* item;

    playlistItem = vlc_playlist_Get(playlist,i);
    item=vlc_playlist_item_GetMedia(vlc_playlist_Get(playlist,i));
    
    //filename
    msg_Info(intfa,"Song Pos: %d",i);
    strcat(output,"file: ");strcat(output,item->psz_name);
    strcat(output,"\n");
    f=fopen(path,"a+"); fputs(output,f); fclose(f); msg_Info(intfa,"\n%s",output); bzero(output,strlen(output));

    //artist && albumartist
    char* artist=input_item_GetArtist(item); strcat(output,"Artist: ");
    if(artist!=NULL)
    sprintf(output,"Artist: %s",artist);
    strcat(output,"\n");
    f=fopen(path,"a+"); fputs(output,f); fclose(f);/*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
    char* albumartist=input_item_GetAlbumArtist(item);strcat(output,"AlbumArtist: ");
    if(albumartist!=NULL)
    sprintf(output,"AlbumArtist: %s",albumartist);
    strcat(output,"\n");
    f=fopen(path,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));

    //title && album
    char* title=input_item_GetTitle(item);strcat(output,"Title: ");
    if(artist!=NULL)
    sprintf(output,"Title: %s",title);
    strcat(output,"\n");
    f=fopen(path,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
    char* album=input_item_GetAlbum(item);strcat(output,"Album: ");
    if(artist!=NULL)
    sprintf(output,"Album: %s",album);
    strcat(output,"\n");
    f=fopen(path,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
    
    //tracknumber && date
    char* track=input_item_GetTrackNumber(item);strcat(output,"Track: ");
    if(track!=NULL)
    sprintf(output,"Track: %s",track);
    strcat(output,"\n");
    f=fopen(path,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
    char* date=input_item_GetDate(item);strcat(output,"Date: ");
    if(date!=NULL)
    sprintf(output,"Date: %s",date);
    strcat(output,"\n");
    f=fopen(path,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
    
    //genre && disc
    char* genre=input_item_GetGenre(item);strcat(output,"Genre: ");
    if(genre!=NULL)
    sprintf(output,"Genre: %s",genre);
    strcat(output,"\n");
    f=fopen(path,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
    char* disc=input_item_GetDiscNumber(item);strcat(output,"Disc: ");
    if(disc!=NULL)
    sprintf(output,"Disc: %s",disc);
    strcat(output,"\n");
    f=fopen(path,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
    
    //time && duration
    vlc_tick_t tick=input_item_GetDuration(item); SEC_FROM_VLC_TICK(tick); 
    float duration=(float)(tick)/1000000.0; int time=duration;
    strcat(output,"Time: "); sprintf(intString, "%d", time);
    strcat(output,intString);strcat(output,"\n"); /*msg_Info(intfa,"\n%s",output);*/ bzero(intString,strlen(intString));
    strcat(output,"Duration: "); sprintf(intString, "%f", duration);
    strcat(output,intString);strcat(output,"\n"); /*msg_Info(intfa,"\n%s",output);*/ bzero(intString,strlen(intString));
    f = fopen (path,"a+"); fputs(output,f); fclose(f); bzero(output,strlen(output)); 
    
    //Position && ID
    int pos=i; sprintf(intString, "%d", pos); 
    strcat(output,"pos: ");strcat(output,intString);strcat(output,"\n");
    f=fopen(path,"a+"); fputs(output,f); fclose(f); msg_Info(intfa,"\n%s",output); bzero(output,strlen(output));
    bzero(intString,strlen(intString));
    int id=vlc_playlist_item_GetId(playlistItem); sprintf(intString, "%d", id); 
    strcat(output,"id: ");strcat(output,intString);strcat(output,"\n");
    f=fopen(path,"a+"); fputs(output,f); fclose(f); msg_Info(intfa,"\n%s",output); bzero(output,strlen(output));
    bzero(intString,strlen(intString));

    playlistItem=NULL;item=NULL;
}
char *playlistinfo(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char* output;
    
    char *fileWrite = malloc(sizeof(char) * 64);bzero(fileWrite, 64);
    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt");

    char *ifColonPresent = strchr(v.data[0],':');
    
    if(v.size==0) //No Argument
    {
        vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;

        vlc_playlist_Lock(playlist);
        for(int i=0;i<vlc_playlist_Count(playlist);i++)
        playlistInfo(intfa,playlist,fileWrite,f,path,i);
        vlc_playlist_Unlock(playlist);

        strcat(fileWrite, "OK\n"); f = fopen (path,"a+"); fputs(fileWrite,f); fclose(f);
        output=strdup("file"); //denoting file mode of output;
    }
    else if(v.size==1) // 1 Argument
    {
        if(ifColonPresent!=NULL) //Argument=[Start:End]
        {
            int start=0,end=0,ind=0;
            //checking argument validity 
            while(v.data[0][ind]!='\0')
            {
                if( ((int)(v.data[0][ind])>=48) && ((int)(v.data[0][ind])<=58)) //either digit or : symbol(ascii=58) 
                ind++;
                else //not valid argument
                {
                    output=strdup("Error: Invalid Type/Number of Arguments.");
                    destroyVector(&v); return (char *)output;
                }
            }
            //separating before and after :
            ind=0;
            while(v.data[0][ind]!=':')
            {ind++;}
            //extracting numbers from arguments
            char* beforeColon[5]; strncpy(beforeColon,v.data[0],ind); start=atoi(beforeColon);
            ifColonPresent++; end=atoi(ifColonPresent);
            
            //playlist info
            vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
            vlc_playlist_Lock(playlist);
            int size=vlc_playlist_Count(playlist);
            if((start>-1)&&(end<=size)&&(start<end)) //Valid Positions
            {
                for(int i=start;i<end;i++)
                playlistInfo(intfa,playlist,fileWrite,f,path,i);
            
                strcat(fileWrite, "OK\n"); f = fopen (path,"a+"); fputs(fileWrite,f); fclose(f);
                output=strdup("file"); //denoting file mode of output;
            }
            else //Invalid Positions
            {
                output=strdup("Error: Bad Position/Index.");
                //msg_Info(intfa,"Error: Bad Position/Index.");
            }
            vlc_playlist_Unlock(playlist);
        }
        else if(ifColonPresent==NULL) //Argument=[Pos]
        {
            int pos=atoi(v.data[0]);

            vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
            vlc_playlist_Lock(playlist);
            int size=vlc_playlist_Count(playlist);
            if((pos<size)&&(pos>-1)) //Valid Positions
            {
                for(int i=pos;i<(pos+1);i++)
                playlistInfo(intfa,playlist,fileWrite,f,path,i);

                strcat(fileWrite, "OK\n"); f = fopen (path,"a+"); fputs(fileWrite,f); fclose(f);
                output=strdup("file"); //denoting file mode of output;
            }
            else //Invalid Positions
            {
                output=strdup("Error: Bad Position/Index.");
                //msg_Info(intfa,"Error: Bad Position/Index.");
            }
            vlc_playlist_Unlock(playlist);
        }
    }
    else //Invalid Argument
    {
        output=strdup("Error: Invalid Type/Number of Arguments.");
        //msg_Info(intfa,Error: Invalid Type/Number of Arguments.");
    }
    free(fileWrite); free(path);

    destroyVector(&v); return (char *)output;
}
char *playlistid(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    char *fileWrite = malloc(sizeof(char) * 64);bzero(fileWrite, 64);
    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt");

    if(v.size==0) //No Argument
    {
        vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;

        vlc_playlist_Lock(playlist);
        int size=vlc_playlist_Count(playlist);
        for(int i=0;i<vlc_playlist_Count(playlist);i++)
        playlistInfo(intfa,playlist,fileWrite,f,path,i);
        vlc_playlist_Unlock(playlist);

        strcat(fileWrite, "OK\n"); f = fopen (path,"a+"); fputs(fileWrite,f); fclose(f);
        output=strdup("file"); //denoting file mode of output;
    }
    else if(v.size==1) //1 Argument
    {
        int id=atoi(v.data[0]);

        vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
        vlc_playlist_Lock(playlist);
        int pos=vlc_playlist_IndexOfId(playlist, id);
        if(pos!=-1) //ID Exists
        {
            for(int i=pos;i<(pos+1);i++)
            playlistInfo(intfa,playlist,fileWrite,f,path,i);

            strcat(fileWrite, "OK\n"); f = fopen (path,"a+"); fputs(fileWrite,f); fclose(f);
            output=strdup("file"); //denoting file mode of output;
        }
        else // ID Doesn't Exists
        {
            output=strdup("Error: Not Valid ID");
            //msg_Info(intfa,"Error: Not Valid ID");
        }
        vlc_playlist_Unlock(playlist);
    }
    else //Invalid Argument
    {
        output=strdup("Error: Invalid Type/Number of Arguments.");
        //msg_Info(intfa,Error: Invalid Type/Number of Arguments.");
    }
    free(fileWrite); free(path);

    destroyVector(&v); return (char *)output;
}

char *playlistfind(intf_thread_t *intfa, char *arguments)
{
    //this command is available for music database(in section 6); not for files currently in queue
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *playlistsearch(intf_thread_t *intfa, char *arguments)
{
    //this command is available for music database(in section 6); not for files currently in queue
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *plchanges(intf_thread_t *intfa, char *arguments) //Playlist Version Not Stored
{
    //VLC doesn't maintain different versions/ changes in a playlist
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *plchangesposid(intf_thread_t *intfa, char *arguments) //Playlist Version Not Stored
{
    //VLC doesn't maintain different versions/ changes in a playlist
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *prio(intf_thread_t *intfa, char *arguments)//Priority Not Maintained for the Playlist
{
    //Priority Not Maintained for the Playlist
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *prioid(intf_thread_t *intfa, char *arguments)//Priority Not Maintained for the Playlist
{
    //Priority Not Maintained for the Playlist
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *rangeid(intf_thread_t *intfa, char *arguments)
{
    //Can't before-hand decide which portion of files to play. Set A-B loop works only for current song in VLC, and vice-versa for MPD
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}

char *shuffle(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;

    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    vlc_playlist_Shuffle(playlist);
    vlc_playlist_Unlock(playlist);
    
    output=strdup("OK\n");
    destroyVector(&v); return (char *)output;
}
char *swap(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    if(v.size==2)
    {
        int bef=atoi(v.data[0]); int aft=atoi(v.data[1]);
        if(bef>aft)
        {
            int temp=bef; bef=aft; aft=temp;
        }
        vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
        vlc_playlist_Lock(playlist);
        vlc_playlist_MoveOne(playlist,bef,aft-1);
        vlc_playlist_MoveOne(playlist,aft,bef); 
        vlc_playlist_Unlock(playlist);

        output=strdup("OK\n");
    }
    else
    {
        output=strdup("Error: Invalid Type/Number of Arguments.");
        //msg_Info(intfa,Error: Invalid Type/Number of Arguments.");
    }

    destroyVector(&v); return (char *)output;
}
char *swapid(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    if(v.size==2)
    {
        int id1=atoi(v.data[0]); int id2=atoi(v.data[1]);
        
        vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
        vlc_playlist_Lock(playlist);
        int bef=vlc_playlist_IndexOfId(playlist,id1); int aft=vlc_playlist_IndexOfId(playlist,id2);
        if(bef>aft)
        {
            int temp=bef; bef=aft; aft=temp;
        }
        vlc_playlist_MoveOne(playlist,bef,aft-1);
        vlc_playlist_MoveOne(playlist,aft,bef);   
        vlc_playlist_Unlock(playlist);

        output=strdup("OK\n");
        //msg_Info(intfa, "Clear %s.", output);
    }
    else
    {
        output=strdup("Error: Invalid Type/Number of Arguments.");
        //msg_Info(intfa,Error: Invalid Type/Number of Arguments.");
    }

    destroyVector(&v); return (char *)output;
}

//tag commands ->volatile changes ->how to add them dynamically without modifying the song(media) file
char *addtagid(intf_thread_t *intfa, char *arguments)
{
    //VLC feature for adding tags dynamically
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *cleartagid(intf_thread_t *intfa, char *arguments)
{
    //VLC feature for adding tags dynamically
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
#pragma endregion

// Playlists and Media Library(Music Database of MPD) Commands
#pragma region
//5. Stored Playlists
char *listplaylist(intf_thread_t *intfa, char *arguments)
{
    // Implemented in playlist
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *listplaylistinfo(intf_thread_t *intfa, char *arguments)
{
    // Implemented in playlistinfo and playlistid
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *listplaylists(intf_thread_t *intfa, char *arguments)
{
    // Implemented in playlist
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}

char *save(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;

    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    char* filename=malloc(sizeof(char)*256);bzero(filename,256);
    filename=vlc_getcwd();
    strcat(filename,"/playlists/"); strcat(filename,v.data[0]); strcat(filename,".m3u");
    vlc_playlist_Export (playlist, filename, "export-m3u");
    vlc_playlist_Unlock(playlist);
    free(filename);
    
    output=strdup("OK\n");    
    destroyVector(&v); return (char *)output;
}
char *load(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    enum input_item_type_e i_type=ITEM_TYPE_PLAYLIST; enum input_item_net_type i_net=ITEM_NET_UNKNOWN;
    input_item_t * item= input_item_NewExt (v.data[0],NULL,NULL,i_type,i_net);

    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    vlc_playlist_Preparse (playlist,item);
    vlc_playlist_AppendOne (playlist,item);
    vlc_playlist_Unlock(playlist);
    
    output=strdup("OK\n");
    destroyVector(&v); return (char *)output;
}

// Changes Not reflected, Unable to save the playlists with changed name in the directory.
char *rm(intf_thread_t *intfa, char *arguments) //playlist not saved
{
    // Removing the playlist from queue
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *renameF(intf_thread_t *intfa, char *arguments) //playlist not saved
{
    // Renaming the playlist from queue
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}

char *playlistadd(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    enum input_item_type_e i_type=ITEM_TYPE_PLAYLIST; enum input_item_net_type i_net=ITEM_NET_UNKNOWN;
    input_item_t * item= input_item_NewExt (v.data[0],NULL,NULL,i_type,i_net);

    char* uri=v.data[1]; char* path = vlc_uri2path(uri);
    char* name=strrchr(uri,'/'); name++;
    input_item_t *media=input_item_New(uri,name);
    
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    
    vlc_playlist_Preparse (playlist,item);
    vlc_playlist_AppendOne (playlist,item);
    vlc_playlist_InsertOne(playlist,vlc_playlist_Count(playlist),media);
    vlc_playlist_Export (playlist, v.data[0], "export-m3u");
    
    vlc_playlist_Unlock(playlist);
    
    output=strdup("OK\n");    
    destroyVector(&v); return (char *)output;
}
char *playlistclear(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    enum input_item_type_e i_type=ITEM_TYPE_PLAYLIST; enum input_item_net_type i_net=ITEM_NET_UNKNOWN;
    input_item_t * item= input_item_NewExt (v.data[0],NULL,NULL,i_type,i_net);
    
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    
    vlc_playlist_Preparse (playlist,item);
    vlc_playlist_AppendOne (playlist,item);
    vlc_playlist_Clear(playlist);
    vlc_playlist_Export (playlist, v.data[0], "export-m3u");
    
    vlc_playlist_Unlock(playlist);
    
    output=strdup("OK\n"); 
    destroyVector(&v); return (char *)output;
}
char *playlistdelete(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    enum input_item_type_e i_type=ITEM_TYPE_PLAYLIST; enum input_item_net_type i_net=ITEM_NET_UNKNOWN;
    input_item_t * item= input_item_NewExt (v.data[0],NULL,NULL,i_type,i_net);
    int pos=atoi(v.data[1]);
    
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    
    vlc_playlist_Preparse (playlist,item);
    vlc_playlist_AppendOne (playlist,item);
    vlc_playlist_RemoveOne(playlist,pos);
    vlc_playlist_Export (playlist, v.data[0], "export-m3u");
    
    vlc_playlist_Unlock(playlist);
    
    output=strdup("OK\n"); 
    destroyVector(&v); return (char *)output;
}
char *playlistmove(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output;
    
    enum input_item_type_e i_type=ITEM_TYPE_PLAYLIST; enum input_item_net_type i_net=ITEM_NET_UNKNOWN;
    input_item_t * item= input_item_NewExt (v.data[0],NULL,NULL,i_type,i_net);
    int pos_from=atoi(v.data[1]); int pos_to=atoi(v.data[2]);
    
    vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
    vlc_playlist_Lock(playlist);
    
    vlc_playlist_Preparse (playlist,item);
    vlc_playlist_AppendOne (playlist,item);
    vlc_playlist_MoveOne (playlist,pos_from,pos_to);
    vlc_playlist_Export (playlist, v.data[0], "export-m3u");
    
    vlc_playlist_Unlock(playlist);
    
    output=strdup("OK\n"); 
    destroyVector(&v); return (char *)output;
}

//6. Music Database
//-> multiple tag features not implemented because using media library, at a time, only 1 tag can be searched using query_params
//-> also 3 MPD tags are available: Artist, Album, Genre. If more tag features are added in media library, they can be extracted and used in MPD later.
void databaseLookup(intf_thread_t *intfa, char* type, vlc_ml_query_params_t* params, FILE *f, char* path, bool addToPlaylist)
{
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    if((!strcmp(type,"artist"))||(!strcmp(type,"Artist")))
    {
        vlc_ml_artist_list_t *ml_artist_list = vlc_ml_list_artists(ml,params,false);
        vlc_ml_artist_t *ml_artist; char* string;
        for(int i=0;i<ml_artist_list->i_nb_items;i++)
        {
            ml_artist=&ml_artist_list->p_items[i];
            msg_Info(intfa,"%s",ml_artist->psz_name);
            if(addToPlaylist)
            {
                vlc_ml_media_list_t *ml_media_list=vlc_ml_list_artist_tracks(ml,params,ml_artist->i_id);
                vlc_ml_media_t *ml_media; input_item_t *media;
                vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
                vlc_playlist_Lock(playlist);
                for(int j=0;j<ml_media_list->i_nb_items;j++)
                {
                    ml_media=&ml_media_list->p_items[j];
                    media=vlc_ml_get_input_item(ml,ml_media->i_id);
                    vlc_playlist_InsertOne(playlist,vlc_playlist_Count(playlist),media);
                    input_item_Release(media); 
                }
                vlc_playlist_Unlock(playlist);
            }
            string=strdup(ml_artist->psz_name); strcat(string,"\n");
            f = fopen (path,"a+"); fputs(string,f); fclose(f);/*msg_Info(intfa,"%s",output);*/
            bzero(string,strlen(string)); free(string);
        }
    }
    else if((!strcmp(type,"genre"))||(!strcmp(type,"Genre")))
    {
        vlc_ml_genre_list_t *ml_genre_list = vlc_ml_list_genres(ml,params);
        vlc_ml_genre_t *ml_genre; char* string;
        for(int i=0;i<ml_genre_list->i_nb_items;i++)
        {
            ml_genre=&ml_genre_list->p_items[i];
            msg_Info(intfa,"%s",ml_genre->psz_name);
            if(addToPlaylist)
            {
                vlc_ml_media_list_t *ml_media_list=vlc_ml_list_genre_tracks(ml,params,ml_genre->i_id);
                vlc_ml_media_t *ml_media; input_item_t *media;
                vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
                vlc_playlist_Lock(playlist);
                for(int j=0;j<ml_media_list->i_nb_items;j++)
                {
                    ml_media=&ml_media_list->p_items[j];
                    media=vlc_ml_get_input_item(ml,ml_media->i_id);
                    vlc_playlist_InsertOne(playlist,vlc_playlist_Count(playlist),media);
                     input_item_Release(media); 
                }
                vlc_playlist_Unlock(playlist);
            }
            string=strdup(ml_genre->psz_name);
            strcat(string,"\n");
            f = fopen (path,"a+"); fputs(string,f); fclose(f);/*msg_Info(intfa,"%s",output);*/
            bzero(string,strlen(string)); free(string);
        }
    }
    else if((!strcmp(type,"album"))||(!strcmp(type,"Album")))
    {
        vlc_ml_album_list_t *ml_album_list = vlc_ml_list_albums(ml,params);
        vlc_ml_album_t *ml_album;char* string;
        for(int i=0;i<ml_album_list->i_nb_items;i++)
        {
            ml_album=&ml_album_list->p_items[i];
            msg_Info(intfa,"%s",ml_album->psz_title);
            if(addToPlaylist)
            {
                vlc_ml_media_list_t *ml_media_list=vlc_ml_list_album_tracks(ml,params,ml_album->i_id);
                vlc_ml_media_t *ml_media; input_item_t *media;
                vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
                vlc_playlist_Lock(playlist);
                for(int j=0;j<ml_media_list->i_nb_items;j++)
                {
                    ml_media=&ml_media_list->p_items[j];
                    media=vlc_ml_get_input_item(ml,ml_media->i_id);
                    vlc_playlist_InsertOne(playlist,vlc_playlist_Count(playlist),media);
                     input_item_Release(media); 
                }
                vlc_playlist_Unlock(playlist);
            }
            string=strdup(ml_album->psz_title);
            strcat(string,"\n");
            f = fopen (path,"a+"); fputs(string,f); fclose(f);/*msg_Info(intfa,"%s",output);*/
            bzero(string,strlen(string)); free(string);
        }
    } 
}
char *find(intf_thread_t *intfa, char *arguments)//[sort {TYPE}] [window {START:END}] Arguments Not Accepted
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output; 
    
    char* type=v.data[0]; char* value=v.data[1];
    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt");
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t param = vlc_ml_query_params_create();
    vlc_ml_query_params_t* params = &param;
    params->psz_pattern=value;

    if((!strcmp(type,"artist"))||(!strcmp(type,"Artist"))||(!strcmp(type,"genre"))||(!strcmp(type,"Genre"))||(!strcmp(type,"album"))||(!strcmp(type,"Album")))
    {
        databaseLookup(intfa,type,params,f,path,false);
        f = fopen (path,"a+"); fputs("OK\n",f); fclose(f);
        output=strdup("file"); 
    }
    else
    {
        output=strdup("Error: Invalid Type/Number of Arguments.");
    }
    free(path);
    destroyVector(&v); return (char *)output;
}
char *findadd(intf_thread_t *intfa, char *arguments)//[sort {TYPE}] [window {START:END}] Arguments Not Accepted
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output; 
    
    char* type=v.data[0]; char* value=v.data[1];
    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt"); 
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t param = vlc_ml_query_params_create();
    vlc_ml_query_params_t* params = &param;
    params->psz_pattern=value;

    if((!strcmp(type,"artist"))||(!strcmp(type,"Artist"))||(!strcmp(type,"genre"))||(!strcmp(type,"Genre"))||(!strcmp(type,"album"))||(!strcmp(type,"Album")))
    {
        databaseLookup(intfa,type,params,f,path,true);
        f = fopen (path,"a+"); fputs("OK\n",f); fclose(f);
        output=strdup("file"); 
    }
    else
    {
        output=strdup("Error: Invalid Type/Number of Arguments.");
    }
    free(path);
    destroyVector(&v); return (char *)output;
}
char *search(intf_thread_t *intfa, char *arguments)//{filter} and {group: grouptype} Arguments Not Accepted
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output; 
    
    char* type=v.data[0]; char* value=v.data[1];
    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt"); 
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t param = vlc_ml_query_params_create();
    vlc_ml_query_params_t* params = &param;
    params->psz_pattern=value;

    if((!strcmp(type,"artist"))||(!strcmp(type,"Artist"))||(!strcmp(type,"genre"))||(!strcmp(type,"Genre"))||(!strcmp(type,"album"))||(!strcmp(type,"Album")))
    {
        databaseLookup(intfa,type,params,f,path,false);
        f = fopen (path,"a+"); fputs("OK\n",f); fclose(f);
        output=strdup("file"); 
    }
    else
    {
        output=strdup("Error: Invalid Type/Number of Arguments.");
    }
    free(path);
    destroyVector(&v); return (char *)output;
}
char *searchadd(intf_thread_t *intfa, char *arguments)//{filter} and {group: grouptype} Arguments Not Accepted
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output; 
    
    char* type=v.data[0]; char* value=v.data[1];
    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt"); 
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t param = vlc_ml_query_params_create();
    vlc_ml_query_params_t* params = &param;
    params->psz_pattern=value;

    if((!strcmp(type,"artist"))||(!strcmp(type,"Artist"))||(!strcmp(type,"genre"))||(!strcmp(type,"Genre"))||(!strcmp(type,"album"))||(!strcmp(type,"Album")))
    {
        databaseLookup(intfa,type,params,f,path,true);
        f = fopen (path,"a+"); fputs("OK\n",f); fclose(f);
        output=strdup("file"); 
    }
    else
    {
        output=strdup("Error: Invalid Type/Number of Arguments.");
    }
    free(path);
    destroyVector(&v); return (char *)output;
}
char *searchaddpl(intf_thread_t *intfa, char *arguments)//{filter} and {group: grouptype} Arguments Not Accepted
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output; 
    
    char* type=v.data[0]; char* value=v.data[1];
    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt"); 
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t param = vlc_ml_query_params_create();
    vlc_ml_query_params_t* params = &param;
    params->psz_pattern=value;

    if((!strcmp(type,"artist"))||(!strcmp(type,"Artist"))||(!strcmp(type,"genre"))||(!strcmp(type,"Genre"))||(!strcmp(type,"album"))||(!strcmp(type,"Album")))
    {
        databaseLookup(intfa,type,params,f,path,true);
        f = fopen (path,"a+"); fputs("OK\n",f); fclose(f);
        output=strdup("file"); 
    }
    else
    {
        output=strdup("Error: Invalid Type/Number of Arguments.");
    }
    free(path);
    destroyVector(&v); return (char *)output;
}

// count TAGTYPE; list TAGTYPE;
char *count(intf_thread_t *intfa, char *arguments)//{multiple filter} and {group: grouptype} Arguments Not Accepted
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output; 
    
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t param = vlc_ml_query_params_create();
    vlc_ml_query_params_t* params = &param;
    char* type=v.data[0];
    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt");

    if((!strcmp(type,"artist"))||(!strcmp(type,"Artist")))
    {
        vlc_ml_artist_list_t *ml_artist_list = vlc_ml_list_artists(ml,params,false);
        vlc_ml_artist_t *ml_artist;char* string;
        for(int i=0;i<ml_artist_list->i_nb_items;i++)
        {
            ml_artist=&ml_artist_list->p_items[i];
            msg_Info(intfa,"%s",ml_artist->psz_name);
            string=strdup(ml_artist->psz_name);
            strcat(string,"\n");
            f = fopen (path,"a+"); fputs(string,f); fclose(f);/*msg_Info(intfa,"%s",output);*/
            bzero(string,strlen(string)); free(string);
        }
    }
    else if((!strcmp(type,"genre"))||(!strcmp(type,"Genre")))
    {
        vlc_ml_genre_list_t *ml_genre_list = vlc_ml_list_genres(ml,params);
        vlc_ml_genre_t *ml_genre; char* string;
        for(int i=0;i<ml_genre_list->i_nb_items;i++)
        {
            ml_genre=&ml_genre_list->p_items[i];
            msg_Info(intfa,"%s",ml_genre->psz_name);
            string=strdup(ml_genre->psz_name);
            strcat(string,"\n");
            f = fopen (path,"a+"); fputs(string,f); fclose(f);/*msg_Info(intfa,"%s",output);*/
            bzero(string,strlen(string)); free(string);
        }
    }
    else if((!strcmp(type,"album"))||(!strcmp(type,"Album")))
    {
        vlc_ml_album_list_t *ml_album_list = vlc_ml_list_albums(ml,params);
        vlc_ml_album_t *ml_album;char* string;
        for(int i=0;i<ml_album_list->i_nb_items;i++)
        {
            ml_album=&ml_album_list->p_items[i];
            msg_Info(intfa,"%s",ml_album->psz_title);
            string=strdup(ml_album->psz_title);
            strcat(string,"\n");
            f = fopen (path,"a+"); fputs(string,f); fclose(f);/*msg_Info(intfa,"%s",output);*/
            bzero(string,strlen(string)); free(string);
        }
    }
    else
    {
        output=strdup("Error: Invalid Type/Number of Arguments.");
        destroyVector(&v); return (char *)output;
    }
    free(path);

    output=strdup("file");
    destroyVector(&v); return (char *)output;
}
char *list(intf_thread_t *intfa, char *arguments) //{filter} and {group: grouptype} Arguments Not Accepted 
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output; 
    
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t param = vlc_ml_query_params_create();
    vlc_ml_query_params_t* params = &param;
    char* type=v.data[0];
    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt"); 

    if((!strcmp(type,"artist"))||(!strcmp(type,"Artist")))
    {
        vlc_ml_artist_list_t *ml_artist_list = vlc_ml_list_artists(ml,params,false);
        vlc_ml_artist_t *ml_artist;char* string;
        for(int i=0;i<ml_artist_list->i_nb_items;i++)
        {
            ml_artist=&ml_artist_list->p_items[i];
            msg_Info(intfa,"%s",ml_artist->psz_name);
            string=strdup(ml_artist->psz_name);
            strcat(string,"\n");
            f = fopen (path,"a+"); fputs(string,f); fclose(f);/*msg_Info(intfa,"%s",output);*/
            bzero(string,strlen(string)); free(string);
        }
    }
    else if((!strcmp(type,"genre"))||(!strcmp(type,"Genre")))
    {
        vlc_ml_genre_list_t *ml_genre_list = vlc_ml_list_genres(ml,params);
        vlc_ml_genre_t *ml_genre; char* string;
        for(int i=0;i<ml_genre_list->i_nb_items;i++)
        {
            ml_genre=&ml_genre_list->p_items[i];
            msg_Info(intfa,"%s",ml_genre->psz_name);
            string=strdup(ml_genre->psz_name);
            strcat(string,"\n");
            f = fopen (path,"a+"); fputs(string,f); fclose(f);/*msg_Info(intfa,"%s",output);*/
            bzero(string,strlen(string)); free(string);
        }
    }
    else if((!strcmp(type,"album"))||(!strcmp(type,"Album")))
    {
        vlc_ml_album_list_t *ml_album_list = vlc_ml_list_albums(ml,params);
        vlc_ml_album_t *ml_album;char* string;
        for(int i=0;i<ml_album_list->i_nb_items;i++)
        {
            ml_album=&ml_album_list->p_items[i];
            msg_Info(intfa,"%s",ml_album->psz_title);
            string=strdup(ml_album->psz_title);
            strcat(string,"\n");
            f = fopen (path,"a+"); fputs(string,f); fclose(f);/*msg_Info(intfa,"%s",output);*/
            bzero(string,strlen(string)); free(string);
        }
    }
    else
    {
        output=strdup("Error: Invalid Type/Number of Arguments.");
        destroyVector(&v); return (char *)output;
    }
    free(path);

    output=strdup("file");
    destroyVector(&v); return (char *)output;
}

// listall(deprecated)=listfiles; listallinfo(deprecated)=lsinfo
void listRecursively(intf_thread_t *intfa, char *basePath, FILE *f, char* filePath, bool metadata)
{
    char path[1000]; struct dirent *dp; DIR *dir = opendir(basePath);

    if (!dir)
    return;
    else
    {
        while ((dp = readdir(dir)) != NULL)
        {
            if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
            {
                msg_Info(intfa,"Path: %s",path);
                strcpy(path, basePath); strcat(path,"/"); strcat(path, dp->d_name);
                struct stat st_buf; int status = stat (path, &st_buf);
                char *output = malloc(sizeof(char) * 512); bzero(output, 512); 

                if (status==0 && S_ISREG (st_buf.st_mode)) //File
                {
                    msg_Info(intfa,"file");
                    strcat(output,"file: "); strcat(output,path); strcat(output,"\n");
                    //if metadata == true, metadata is extracted from the song and added
                    if(metadata==false)
                    {
                        f = fopen (filePath,"a+"); fputs(output,f); fclose(f);    
                    }
                    else
                    {
                        f = fopen (filePath,"a+"); fputs(output,f); fclose(f); bzero(output,strlen(output));  
                        char* uri=vlc_path2uri(path,"file"); char* name=strrchr(uri,'/'); name++;
                        input_item_t *item=input_item_New(uri,name);
                        char *intString[4]; 

                        //filename
                        strcat(output,"file: ");strcat(output,item->psz_name); strcat(output,"\n");
                        f = fopen (filePath,"a+"); fputs(output,f); fclose(f); msg_Info(intfa,"\n%s",output); bzero(output,strlen(output));

                        //artist && albumartist
                        char* artist=input_item_GetArtist(item); strcat(output,"Artist: ");
                        if(artist!=NULL)
                        sprintf(output,"Artist: %s",artist);
                        strcat(output,"\n");
                        f = fopen (filePath,"a+"); fputs(output,f); fclose(f);/*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
                        char* albumartist=input_item_GetAlbumArtist(item);strcat(output,"AlbumArtist: ");
                        if(albumartist!=NULL)
                        sprintf(output,"AlbumArtist: %s",albumartist);
                        strcat(output,"\n");
                        f = fopen (filePath,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));

                        //title && album
                        char* title=input_item_GetTitle(item);strcat(output,"Title: ");
                        if(artist!=NULL)
                        sprintf(output,"Title: %s",title);
                        strcat(output,"\n");
                        f = fopen (filePath,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
                        char* album=input_item_GetAlbum(item);strcat(output,"Album: ");
                        if(artist!=NULL)
                        sprintf(output,"Album: %s",album);
                        strcat(output,"\n");
                        f = fopen (filePath,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
                        
                        //tracknumber && date
                        char* track=input_item_GetTrackNumber(item);strcat(output,"Track: ");
                        if(track!=NULL)
                        sprintf(output,"Track: %s",track);
                        strcat(output,"\n");
                        f = fopen (filePath,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
                        char* date=input_item_GetDate(item);strcat(output,"Date: ");
                        if(date!=NULL)
                        sprintf(output,"Date: %s",date);
                        strcat(output,"\n");
                        f = fopen (filePath,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
                        
                        //genre && disc
                        char* genre=input_item_GetGenre(item);strcat(output,"Genre: ");
                        if(genre!=NULL)
                        sprintf(output,"Genre: %s",genre);
                        strcat(output,"\n");
                        f = fopen (filePath,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
                        char* disc=input_item_GetDiscNumber(item);strcat(output,"Disc: ");
                        if(disc!=NULL)
                        sprintf(output,"Disc: %s",disc);
                        strcat(output,"\n");
                        f = fopen (filePath,"a+"); fputs(output,f); fclose(f); /*msg_Info(intfa,"\n%s",output);*/ bzero(output,strlen(output));
                        
                        //time && duration
                        vlc_tick_t tick=input_item_GetDuration(item); SEC_FROM_VLC_TICK(tick); 
                        float duration=(float)(tick)/1000000.0; int time=duration;
                        strcat(output,"Time: "); sprintf(intString, "%d", time);
                        strcat(output,intString);strcat(output,"\n"); /*msg_Info(intfa,"\n%s",output);*/ bzero(intString,strlen(intString));
                        strcat(output,"Duration: "); sprintf(intString, "%f", duration);
                        strcat(output,intString);strcat(output,"\n"); /*msg_Info(intfa,"\n%s",output);*/ bzero(intString,strlen(intString));
                        f = fopen (filePath,"a+"); fputs(output,f); fclose(f); bzero(output,strlen(output)); 
                    }
                }
                else if(status==0 && S_ISDIR (st_buf.st_mode)) //Folder
                {
                    msg_Info(intfa,"folder");
                    strcat(output,"directory: "); strcat(output,path); strcat(output,"\n");
                    f = fopen (filePath,"a+"); fputs(output,f); fclose(f);
                }
                else //Not A File
                {
                    msg_Info(intfa,"nothing");
                }
                bzero(output,strlen(output)); free(output);
                listRecursively(intfa, path,f,filePath,metadata);
            }
        }
    }

    closedir(dir);
}
char *listall(intf_thread_t *intfa, char *arguments)//{URI} Arguments Not Accepted
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output; 
    
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t param = vlc_ml_query_params_create();
    vlc_ml_query_params_t* params = &param;

    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd(); strcat(path,"/temp/file.txt"); 

    vlc_ml_entry_point_list_t* ml_entrypoint_list;
    vlc_ml_entry_point_t* ml_entrypoint; char* mrl; char* pathI;
    vlc_ml_list_folder(ml,&ml_entrypoint_list);
    for(int i=0;i<ml_entrypoint_list->i_nb_items;i++)
    {
        ml_entrypoint=&ml_entrypoint_list->p_items[i];
        mrl=ml_entrypoint->psz_mrl;
        pathI = vlc_uri2path(mrl);
        msg_Info(intfa,"%s\n%s\n%s",mrl,pathI,path);
        listRecursively(intfa,pathI,f,path,false);
        msg_Info(intfa,"asda");
    }
    
    msg_Info(intfa,"aafsdss %s",path);
    f=fopen(path,"a+");
    char* str="OK\n";
    fputs(str,f);
    msg_Info(intfa,"rrdsass");
    fclose(f);
    free(path);
    
    output=strdup("file");
    destroyVector(&v); return (char *)output;
}
char *listallinfo(intf_thread_t *intfa, char *arguments)//{URI} Arguments Not Accepted
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output; 
    
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t param = vlc_ml_query_params_create();
    vlc_ml_query_params_t* params = &param;

    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt"); 
    
    vlc_ml_entry_point_list_t* ml_entrypoint_list;
    vlc_ml_entry_point_t* ml_entrypoint; char* mrl; char* pathI;
    vlc_ml_list_folder(ml,&ml_entrypoint_list);
    for(int i=0;i<ml_entrypoint_list->i_nb_items;i++)
    {
        ml_entrypoint=&ml_entrypoint_list->p_items[i];
        mrl=ml_entrypoint->psz_mrl;
        pathI = vlc_uri2path(mrl);
        listRecursively(intfa,pathI,f,path,true);
    }
    f=fopen(path,"a+"); fputs("OK\n",f); fclose(f);
    free(path);
    
    output=strdup("file");
    destroyVector(&v); return (char *)output;
}
char *listfiles(intf_thread_t *intfa, char *arguments)//{URI} Arguments Not Accepted
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output; 
    
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t param = vlc_ml_query_params_create();
    vlc_ml_query_params_t* params = &param;

    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt"); 
    
    vlc_ml_entry_point_list_t* ml_entrypoint_list;
    vlc_ml_entry_point_t* ml_entrypoint; char* mrl; char* pathI;
    vlc_ml_list_folder(ml,&ml_entrypoint_list);
    for(int i=0;i<ml_entrypoint_list->i_nb_items;i++)
    {
        ml_entrypoint=&ml_entrypoint_list->p_items[i];
        mrl=ml_entrypoint->psz_mrl;
        pathI = vlc_uri2path(mrl);
        listRecursively(intfa,pathI,f,path,false);
    }
    f=fopen(path,"a+"); fputs("OK\n",f); fclose(f);
    free(path);
    
    output=strdup("file");
    destroyVector(&v); return (char *)output;
}
char *lsinfo(intf_thread_t *intfa, char *arguments)//{URI} Arguments Not Accepted
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output; 
    
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t param = vlc_ml_query_params_create();
    vlc_ml_query_params_t* params = &param;

    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt"); 
    
    vlc_ml_entry_point_list_t* ml_entrypoint_list;
    vlc_ml_entry_point_t* ml_entrypoint; char* mrl; char* pathI;
    vlc_ml_list_folder(ml,&ml_entrypoint_list);
    for(int i=0;i<ml_entrypoint_list->i_nb_items;i++)
    {
        ml_entrypoint=&ml_entrypoint_list->p_items[i];
        mrl=ml_entrypoint->psz_mrl;
        pathI = vlc_uri2path(mrl);
        listRecursively(intfa,pathI,f,path,true);
    }
    f=fopen(path,"a+"); fputs("OK\n",f); fclose(f);
    free(path);

    output=strdup("file");
    destroyVector(&v); return (char *)output;
}

//Using same vlc_ml_folder_reload function for update and rescan
char *update(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output; 
    
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t param = vlc_ml_query_params_create();
    vlc_ml_query_params_t* params = &param;

    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt"); 
    
    vlc_ml_entry_point_list_t* ml_entrypoint_list;
    vlc_ml_entry_point_t* ml_entrypoint; char* mrl; char* pathI;
    vlc_ml_list_folder(ml,&ml_entrypoint_list);
    for(int i=0;i<ml_entrypoint_list->i_nb_items;i++)
    {
        ml_entrypoint=&ml_entrypoint_list->p_items[i];
        mrl=ml_entrypoint->psz_mrl;
        vlc_ml_reload_folder(ml,mrl);
    }
    f=fopen(path,"a+"); fputs("OK\n",f); fclose(f);
    free(path);
    
    output=strdup("file");
    destroyVector(&v); return (char *)output;
}
char *rescan(intf_thread_t *intfa, char *arguments)
{
    vect v; vlc_vector_init(&v); getArg(intfa,arguments,&v);
    char *output; 
    
    vlc_medialibrary_t* ml=intfa->p_sys->vlc_medialibrary;
    vlc_ml_query_params_t param = vlc_ml_query_params_create();
    vlc_ml_query_params_t* params = &param;

    FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
    path=vlc_getcwd();strcat(path,"/temp/file.txt"); 
    
    vlc_ml_entry_point_list_t* ml_entrypoint_list;
    vlc_ml_entry_point_t* ml_entrypoint; char* mrl; char* pathI;
    vlc_ml_list_folder(ml,&ml_entrypoint_list);
    for(int i=0;i<ml_entrypoint_list->i_nb_items;i++)
    {
        ml_entrypoint=&ml_entrypoint_list->p_items[i];
        mrl=ml_entrypoint->psz_mrl;
        vlc_ml_reload_folder(ml,mrl);
    }
    f=fopen(path,"a+"); fputs("OK\n",f); fclose(f);
    free(path);
    
    output=strdup("file");
    destroyVector(&v); return (char *)output;
}
//Not Implemented
char *album_art(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *getfingerprint(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *read_comments(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *read_picture(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
#pragma endregion

// Unavailable Commands
#pragma region
//7. Mounts & Neighbours
char *mount(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *unmount(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;;
}
char *listmounts(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *listneighbors(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}

//8. Sticker
char *sticker(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}

//10. Partition commands
char *partition(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *listpartitions(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *newpartition(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *delpartition(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *moveoutput(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}

//11. Audio Output Devices: VLC MPD uses VLC default audio output mode/features
char *enableoutput(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *disableoutput(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *toggleoutput(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *devices(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *outputset(intf_thread_t *intfa, char *arguments)
{
    // Default VLC Output
    char* output=strdup("Output: Default VLC Output\nOK\n");
    return (char *)output;
}

//12. Reflections
char *config(intf_thread_t *intfa, char *arguments)
{
    // Default VLC Configs
    char* output=strdup("Config: Default VLC Configs\nOK\n");
    return (char *)output;
}
char *decoders(intf_thread_t *intfa, char *arguments)
{
    // Default VLC Decoders
    char* output=strdup("Decoders: Default VLC Decoders.\nOK\n");
    return (char *)output;
}
char *commands(intf_thread_t *intfa, char *arguments)
{
    /*fixme: HardCoded, Need a better way to send data*/
    char *output;    
    output=strdup("command: add\ncommand: addid\ncommand: addtagid\ncommand: albumart\ncommand: channels\ncommand: clear\ncommand: clearerror\ncommand: cleartagid\ncommand: close\ncommand: commands\ncommand: config\ncommand: consume\ncommand: count\ncommand: crossfade\ncommand: currentsong\ncommand: decoders\ncommand: delete\ncommand: deleteid\ncommand: disableoutput\ncommand: enableoutput\ncommand: find\ncommand: findadd\ncommand: idle\ncommand: kill\ncommand: list\ncommand: listall\ncommand: listallinfo\ncommand: listfiles\ncommand: listmounts\ncommand: listpartitions\ncommand: listplaylist\ncommand: listplaylistinfo\ncommand: listplaylists\ncommand: load\ncommand: lsinfo\ncommand: mixrampdb\ncommand: mixrampdelay\ncommand: mount\ncommand: move\ncommand: moveid\ncommand: newpartition\ncommand: next\ncommand: notcommands\ncommand: outputs\ncommand: outputset\ncommand: partition\ncommand: password\ncommand: pause\ncommand: ping\ncommand: play\ncommand: playid\ncommand: playlist\ncommand: playlistadd\ncommand: playlistclear\ncommand: playlistdelete\ncommand: playlistfind\ncommand: playlistid\ncommand: playlistinfo\ncommand: playlistmove\ncommand: playlistsearch\ncommand: plchanges\ncommand: plchangesposid\ncommand: previous\ncommand: prio\ncommand: prioid\ncommand: random\ncommand: rangeid\ncommand: readcomments\ncommand: readmessages\ncommand: rename\ncommand: repeat\ncommand: replay_gain_mode\ncommand: replay_gain_status\ncommand: rescan\ncommand: rm\ncommand: save\ncommand: search\ncommand: searchadd\ncommand: searchaddpl\ncommand: seek\ncommand: seekcur\ncommand: seekid\ncommand: sendmessage\ncommand: setvol\ncommand: shuffle\ncommand: single\ncommand: stats\ncommand: status\ncommand: sticker\ncommand: stop\ncommand: subscribe\ncommand: swap\ncommand: swapid\ncommand: tagtypes\ncommand: toggleoutput\ncommand: unmount\ncommand: unsubscribe\ncommand: update\ncommand: urlhandlers\ncommand: volume\nOK\n");
    return (char *)output;
}
char *not_commands(intf_thread_t *intfa, char *arguments)
{
    //all commands, even though not implemented, are handled in the file
    char *output=strdup("OK\n");
    return (char *)output;
}
char *urlhandlers(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}

//13. Client to Client Connections
char *subscribe(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *unsubscribe(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *channels(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *read_messages(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;
}
char *send_message(intf_thread_t *intfa, char *arguments)
{
    // Features Not Available in VLC
    char* output=strdup("Command Not Implemented.\nOK\n");
    return (char *)output;;
}
#pragma endregion

/* Implementation of Helper Function */
#pragma region
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

    // 2. copying argument because strchr returns new pointer
    char *arg = strchr(input, ' ');
    if(arg!=NULL)
    {
        int i = 0;
        while (arg[i] != '\0')
        {
            argument[i] = arg[i];
            i++;
        }
        argument[i] = '\0';
    }
    else
    {
        argument = NULL;
    }
    
    // 3. separate command and argument using whitespace
    if (argument != NULL)
    {
        msg_Info(intfa, "argument present");
        int s = strlen(input);
        strncat(command, input, s - strlen(argument));
        trim(argument);
    }
    else
    {
        msg_Info(intfa, "argument not present");
        int s = strlen(input);
        strncat(command, input, s);
        argument = ""; //empty not null
    }
        
    // 4. check for lowercase - here I am explicity converting to small string
    for (int i = 0; command[i]; i++)
    {
        command[i] = tolower(command[i]);
    }

    // 5. remove " " from arguments if present
    if(argument[0]=='"')
    {
        // msg_Info(intfa, "Argument has Q\n");
        int s=strlen(argument);
        if(argument[s-1]=='"')
        {
            argument[0]=' ';
            argument[s-1]=' ';
            trim(argument);
        }
        else
        {
            argument="\"";
        }
        
    }
    
    // 6. Function Check
    // msg_Info(intfa, "%p %p %p %p", input, command, argument, arg);
    //msg_Info(intfa, "arg:%s.\nargsize:%ld.\n", arg, strlen(arg));
    //msg_Info(intfa, "command:%s.\ncommandSize:%ld.\narguments:%s.\nargumentsSize:%ld.\n", command, strlen(command), argument, strlen(argument));
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
    // 0. Function Start
    intf_thread_t *intfa = (intf_thread_t *)threadArgument;
    intf_sys_t *p_sys = intfa->p_sys;
    msg_Info(intfa, "Hello from thread side!");

    // 1. Initialising Client
    int server_fd = intfa->p_sys->server_fd; 
    struct sockaddr_in clientAddr; int client_fd, clientAddrLen = sizeof(clientAddr);
    
    // 2. Function Variables
    //File Descriptor Sets
    fd_set currentSockets, readySockets;
    FD_ZERO(&currentSockets); FD_ZERO(&readySockets); FD_SET(server_fd, &currentSockets);
    //idle_check_array: to check IDLE status of client
    bool idle_check_array[1024]; //fd_set maximum size: 1024
    for(int i=0;i<1024;i++)
    idle_check_array[i]=false;

    // 3. Listening to clients
    if (listen(server_fd, SOMAXCONN) < 0)
    {
        msg_Info(intfa, "listen");
        exit(EXIT_FAILURE);
    }

    // 4. Event Loop
    while (true)
    {
        readySockets = currentSockets;
        if (select(FD_SETSIZE, &readySockets, NULL, NULL, NULL) < 0)
        {
            msg_Info(intfa, "select");
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
                        msg_Info(intfa, "accept");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(client_fd, &currentSockets);

                    // 1. return OK connection established
                    char *output = strdup("OK MPD Version 0.21.25\n");    /*fixme: ideally vlc version should be here, but mpc client refusing connection for invalid version*/
                    send(client_fd, output, strlen(output), 0);
                    msg_Info(intfa, "server read: %d\n", client_fd);
                    free(output);
                }
                else
                {
                    // 0. client has pending read connection (new message)
                    client_fd = i;
                    msg_Info(intfa, "client_fd read: %d\n", client_fd);

                    // 1. read operation
                    int n;
                    char *input = malloc(sizeof(char) * 4096), *arguments = malloc(sizeof(char) * 2048), *command = malloc(sizeof(char) * 1024);
                    bzero(input, 4096); bzero(arguments, 2048); bzero(command, 1024);
                    if ((n = read(client_fd, input, 4095)) < 0)
                    {
                        msg_Info(intfa, "read");
                        exit(EXIT_FAILURE);
                    }
                    input[strlen(input) - 1] = '\0';

                    // 2. Handle Size = 0 Input
                    if (strlen(input) == 0)
                    {
                        commandFunc *command = searchCommand("commands");
                        char *output = command->commandName(intfa, "arg");//sending dummy arg here, as commands function doesn't require arguments
                        send(client_fd, output, strlen(output), 0);
                        msg_Info(intfa, "Size0 %s", output);
                        free(input); free(command); free(arguments); free(output);
                        break;
                    }
                    
                    // 3. Parse Input: Separate Command and Arguments
                    parseInput(intfa,input,command,arguments);

                    // 4. Handle Argument with Missing Quotation
                    if(arguments[0]=='"')
                    {
                        char* output = strdup("ACK Argument is Missing Quotation\n");
                        send(client_fd, output, strlen(output), 0);
                        free(input); free(command); free(arguments); free(output);
                        msg_Info(intfa,"Argument is Missing Quotation\n");
                        break;
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
                    vlc_player_Unlock(player);
                    
                    // 6. search for the command in the array
                    commandFunc* commandF=searchCommand(command);
                    //Command Does Not Exists
                    if(commandF==NULL)
                    {
                        msg_Info(intfa,"ACK Command Does Not Exist\n");
                        strcat(command, ": Command Doesn't exist\n");
                        send(client_fd, command, strlen(command), 0);
                    }
                    //Manage Special Commands
                    else if(command=="idle")
                    {
                        idle_check_array[client_fd]=true;
                    }
                    else if(command=="noidle")
                    {
                        idle_check_array[client_fd]=false;
                    }
                    else if(command=="close")
                    {
                         //remove client_fd to current
                        FD_CLR(client_fd, &currentSockets);
                    }
                    //Command Exists
                    else
                    {
                        msg_Info(intfa,"ACK Command Does Exist\n");
                        if(idle_check_array[client_fd]) //if client in idle mode, don't respond: ideally the server shouldn't even read message from client side
                        {
                            break;
                        }
                        
                        char* output = commandF->commandName(intfa, arguments);
                        if(!strncmp(output,"Error",5)) //Error in Arguments
                        {
                            msg_Info(intfa,"Invalid Argument\n");
                            strcat(arguments," :Invalid Argument\n");
                            send(client_fd, arguments, strlen(arguments), 0);
                        }
                        else if(!strcmp(output,"file")) //Output Mode: Multiple 'Send' Commands (after reading through file)
                        {
                            msg_Info(intfa,"Multiple Send Output\n");

                            int fileSize=0,ch=0; FILE *f; char* path=malloc(sizeof(char)*256);bzero(path,256);
                            path=vlc_getcwd();strcat(path,"/temp/file.txt");
                            //reading file size
                            // f=fopen(path,"r");
                            // while((ch=fgetc(f))!=EOF)
                            // fileSize++;
                            // msg_Info(intfa,"fileSize: %d",fileSize);
                            // fclose(f);
                            
                            //sending data to socket
                            char buffer[100]; int bytes_read=0, ptr=0;
                            f=fopen(path,"r");
                            while((ch=fgetc(f))!=EOF)
                            {
                                //printf("%c",(char)ch);
                                bytes_read++; buffer[ptr]=ch; ptr++;
                                if(ptr=100)//if fixed buffer length is read
                                {
                                    send(client_fd,buffer,strlen(buffer),0);
                                    ptr=0; bzero(buffer,strlen(buffer));
                                }
                                if(bytes_read==fileSize)//reached the EOF
                                {
                                    send(client_fd,buffer,strlen(buffer),0);
                                    ptr=0; bzero(buffer,strlen(buffer));
                                    msg_Info(intfa,"EOF"); break;
                                }
                            }
                            fclose(f);
                            remove(path); free(path);
                        }
                        else //Output Mode: (Single) 'Send' Command
                        {
                            msg_Info(intfa,"Single Send Output\n");
                            send(client_fd, output, strlen(output), 0);    
                        }
                        free(output);
                    }

                    // 7. free up dynamically allocated data
                    free(input); free(command); free(arguments);
                }
            }
        }
    }

    // 5. Function End
    return NULL;
}

// Open Module
static int Open(vlc_object_t *obj)
{
    // 0. Module Start
    intf_thread_t *intfa = (intf_thread_t *)obj;
    msg_Info(intfa, "Hello World! MPD Server Started");

    // 1. Initialising Server
    int server_fd; struct sockaddr_in serverAddr;

    // 2. Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        msg_Info(intfa, "Error Opening Socket. Socket Failed");
        exit(EXIT_FAILURE);
    }
    bzero((char *)&serverAddr, sizeof(serverAddr));

    // 3. Forcefully attaching socket to the port 6600 - OPTIONAL
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        msg_Info(intfa, "setsockopt");
        exit(EXIT_FAILURE);
    }
    serverAddr.sin_family = AF_INET; serverAddr.sin_addr.s_addr = INADDR_ANY; serverAddr.sin_port = htons(PORT);

    // 4. Binding socket to the port 6600
    if (bind(server_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        msg_Info(intfa, "bind failed");
        exit(EXIT_FAILURE);
    }

    // 5. Thread Creation & System Resource Allocation
    intf_thread_t *mpd_thread = (intf_thread_t *)obj;
    intf_sys_t *p_sys = malloc(sizeof(*p_sys));
    if (unlikely(p_sys == NULL))
    {
        msg_Info(intfa,"p_sys error");
        return VLC_ENOMEM;
    }
    mpd_thread->p_sys = p_sys;
    
    // 6. Initialising VLC Playlist, Player, MediaLibrary. Also adding default MPD Folder here.
    p_sys->server_fd = server_fd;
    p_sys->vlc_playlist = vlc_intf_GetMainPlaylist(mpd_thread);
    vlc_player_t *player = vlc_playlist_GetPlayer(p_sys->vlc_playlist);
    p_sys->vlc_medialibrary = vlc_ml_instance_get(obj);

    // 7. Calling Client Handling Function
    int i = 0;
    if ((i = vlc_clone(&mpd_thread->p_sys->thread, clientHandling, mpd_thread, VLC_THREAD_PRIORITY_LOW)) < 0)
    {
        msg_Info(intfa, "clone failed");
        exit(EXIT_FAILURE);
    }
    
    // 8. Adding Listeners to Player
    static struct vlc_player_cbs const player_cbs = {.on_state_changed = player_on_state_changed};
    vlc_player_Lock(player);
    p_sys->player_listener = vlc_player_AddListener(player, &player_cbs, mpd_thread);
    vlc_player_Unlock(player);

    // 9. Module End
    return VLC_SUCCESS;
}

// Close Module
static void Close(vlc_object_t *obj)
{
    // 0. Module Start
    intf_thread_t *intfa = (intf_thread_t *)obj;
    intf_sys_t *p_sys = intfa->p_sys;

    // 1. Freeing up resources
    // Thread Management
    vlc_cancel(intfa->p_sys->thread);
    vlc_join(intfa->p_sys->thread, NULL);
    
    // Player and Listener Management
    vlc_player_t *player = vlc_playlist_GetPlayer(p_sys->vlc_playlist);
    vlc_player_Lock(player);
    vlc_player_RemoveListener(player, p_sys->player_listener);
    vlc_player_Unlock(player);
    
    // System Resources Management
    free(p_sys);

    // 2. Module End
    msg_Info(intfa, "Good bye MPD Server");
}

// Module Descriptor
vlc_module_begin()
    set_shortname(N_("MPD"))
    set_description(N_("MPD Control Interface Module"))
    set_capability("interface", 0)
    set_callbacks(Open, Close)
    set_category(CAT_INTERFACE)
vlc_module_end()
