#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
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
    char *(*commandName)(intf_thread_t *intf, char *arguments); /*some-variable for passing*/ //i think it would be better to pass an array of arguments
} commandFunc;

/*Function Declarations*/
static int string_compare_function(const void *key, const void *arrayElement);
commandFunc *searchCommand(const char *key);
void trim(char *s);
void *clientHandling(void *arg);
/*MPD Interaction Functions Declarations*/
//client connection handling commands
char *close(intf_thread_t *intfa, char *arguments);
char *ping(intf_thread_t *intfa, char *arguments);
char *password(intf_thread_t *intfa, char *arguments);
char *tagtypes(intf_thread_t *intfa, char *arguments);
//database commands
//C--ommandResult handle_listfiles_db(Client &client, Response &r, const char *uri----);
//C--ommandResult handle_lsinfo2(Client &client, const char *uri, Response &response---);
char *find(intf_thread_t *intfa, char *arguments);
char *findadd(intf_thread_t *intfa, char *arguments);
char *search(intf_thread_t *intfa, char *arguments);
char *searchadd(intf_thread_t *intfa, char *arguments);
char *searchaddpl(intf_thread_t *intfa, char *arguments);
char *count(intf_thread_t *intfa, char *arguments);
char *listall(intf_thread_t *intfa, char *arguments);
char *list(intf_thread_t *intfa, char *arguments);
char *listallinfo(intf_thread_t *intfa, char *arguments);
//file commands - reading music file ??
//C--ommandResult handle_listfiles_local(Response &response, Path path_fs---);
char *read_comments(intf_thread_t *intfa, char *arguments);
char *album_art(intf_thread_t *intfa, char *arguments);
char *read_picture(intf_thread_t *intfa, char *arguments);
//fingerprint commands
char *getfingerprint(intf_thread_t *intfa, char *arguments);
//Message c2c communication
char *subscribe(intf_thread_t *intfa, char *arguments);
char *unsubscribe(intf_thread_t *intfa, char *arguments);
char *channels(intf_thread_t *intfa, char *arguments);
char *read_messages(intf_thread_t *intfa, char *arguments);
char *send_message(intf_thread_t *intfa, char *arguments);
//neighbour commands
char *listneighbors(intf_thread_t *intfa, char *arguments);
//other commands
char *urlhandlers(intf_thread_t *intfa, char *arguments);
char *decoders(intf_thread_t *intfa, char *arguments);
char *kill(intf_thread_t *intfa, char *arguments);
char *listfiles(intf_thread_t *intfa, char *arguments);
char *lsinfo(intf_thread_t *intfa, char *arguments);
char *update(intf_thread_t *intfa, char *arguments);
char *rescan(intf_thread_t *intfa, char *arguments);
char *setvol(intf_thread_t *intfa, char *arguments);
char *volume(intf_thread_t *intfa, char *arguments);
char *stats(intf_thread_t *intfa, char *arguments);
char *config(intf_thread_t *intfa, char *arguments);
char *idle(intf_thread_t *intfa, char *arguments);
//output commands
char *enableoutput(intf_thread_t *intfa, char *arguments);
char *disableoutput(intf_thread_t *intfa, char *arguments);
char *toggleoutput(intf_thread_t *intfa, char *arguments);
char *outputset(intf_thread_t *intfa, char *arguments);
char *devices(intf_thread_t *intfa, char *arguments);
//partition commands
char *partition(intf_thread_t *intfa, char *arguments);
char *listpartitions(intf_thread_t *intfa, char *arguments);
char *newpartition(intf_thread_t *intfa, char *arguments);
char *delpartition(intf_thread_t *intfa, char *arguments);
char *moveoutput(intf_thread_t *intfa, char *arguments);
//player commands
char *play(intf_thread_t *intfa, char *arguments);
char *playid(intf_thread_t *intfa, char *arguments);
char *stop(intf_thread_t *intfa, char *arguments);
char *currentsong(intf_thread_t *intfa, char *arguments);
char *pause(intf_thread_t *intfa, char *arguments);
char *status(intf_thread_t *intfa, char *arguments);
char *next(intf_thread_t *intfa, char *arguments);
char *previous(intf_thread_t *intfa, char *arguments);
char *repeat(intf_thread_t *intfa, char *arguments);
char *single(intf_thread_t *intfa, char *arguments);
char *consume(intf_thread_t *intfa, char *arguments);
char *random(intf_thread_t *intfa, char *arguments);
char *clearerror(intf_thread_t *intfa, char *arguments);
char *seek(intf_thread_t *intfa, char *arguments);
char *seekid(intf_thread_t *intfa, char *arguments);
char *seekcur(intf_thread_t *intfa, char *arguments);
char *crossfade(intf_thread_t *intfa, char *arguments);
char *mixrampdb(intf_thread_t *intfa, char *arguments);
char *mixrampdelay(intf_thread_t *intfa, char *arguments);
char *replay_gain_mode(intf_thread_t *intfa, char *arguments);
char *replay_gain_status(intf_thread_t *intfa, char *arguments);
//playlist commands
char *save(intf_thread_t *intfa, char *arguments);
char *load(intf_thread_t *intfa, char *arguments);
char *listplaylist(intf_thread_t *intfa, char *arguments);
char *listplaylistinfo(intf_thread_t *intfa, char *arguments);
char *rm(intf_thread_t *intfa, char *arguments);
char *rename(intf_thread_t *intfa, char *arguments);
char *playlistdelete(intf_thread_t *intfa, char *arguments);
char *playlistmove(intf_thread_t *intfa, char *arguments);
char *playlistclear(intf_thread_t *intfa, char *arguments);
char *playlistadd(intf_thread_t *intfa, char *arguments);
char *listplaylists(intf_thread_t *intfa, char *arguments);
//queue commands
char *add(intf_thread_t *intfa, char *arguments);
char *addid(intf_thread_t *intfa, char *arguments);
char *rangeid(intf_thread_t *intfa, char *arguments);
char *delete (intf_thread_t *intfa, char *arguments);
char *deleteid(intf_thread_t *intfa, char *arguments);
char *playlist(intf_thread_t *intfa, char *arguments);
char *shuffle(intf_thread_t *intfa, char *arguments);
char *clear(intf_thread_t *intfa, char *arguments);
char *plchanges(intf_thread_t *intfa, char *arguments);
char *plchangesposid(intf_thread_t *intfa, char *arguments);
char *playlistinfo(intf_thread_t *intfa, char *arguments);
char *playlistid(intf_thread_t *intfa, char *arguments);
char *playlistfind(intf_thread_t *intfa, char *arguments);
char *playlistsearch(intf_thread_t *intfa, char *arguments);
char *prio(intf_thread_t *intfa, char *arguments);
char *prioid(intf_thread_t *intfa, char *arguments);
char *move(intf_thread_t *intfa, char *arguments);
char *moveid(intf_thread_t *intfa, char *arguments);
char *swap(intf_thread_t *intfa, char *arguments);
char *swapid(intf_thread_t *intfa, char *arguments);
//sticker commands
char *sticker(intf_thread_t *intfa, char *arguments);
//storage commands
//C--ommandResult handle_listfiles_storage(Response &r, Storage &storage, const char *uri--);
//C--ommandResult handle_listfiles_storage(Client &client, Response &r, const char *uri--);
char *listmounts(intf_thread_t *intfa, char *arguments);
char *mount(intf_thread_t *intfa, char *arguments);
char *unmount(intf_thread_t *intfa, char *arguments);
//tag commands
char *addtagid(intf_thread_t *intfa, char *arguments);
char *cleartagid(intf_thread_t *intfa, char *arguments);

//command array
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
    {"close", close},
    {"commands", commands},
    {"config", config},
    {"consume", consume},
    {"count", count},
    {"crossfade", crossfade},
    {"currentsong", currentsong},
    {"decoders", decoders},
    {"delete", delete},
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
    {"pause", pause},
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
    {"random", random},
    {"rangeid", rangeid},
    {"readcomments", read_comments},
    {"readmessages", read_messages},
    {"readpicture", read_picture},
    {"rename", rename},
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

//functions
//client connection handling commands
char *close(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *ping(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *password(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *tagtypes(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//database commands
char *find(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *findadd(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *search(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *searchadd(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *searchaddpl(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *count(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *listall(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *list(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *listallinfo(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//file commands - reading music file ??
char *read_comments(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *album_art(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *read_picture(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//fingerprint commands
char *getfingerprint(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//Message c2c communication
char *subscribe(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *unsubscribe(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *channels(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *read_messages(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *send_message(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//neighbour commands
char *listneighbors(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//other commands
char *urlhandlers(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *decoders(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *kill(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *listfiles(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *lsinfo(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *update(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *rescan(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *setvol(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *volume(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *stats(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *config(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *idle(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//output commands
char *enableoutput(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *disableoutput(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *toggleoutput(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *outputset(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *devices(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//partition commands
char *partition(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *listpartitions(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *newpartition(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *delpartition(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *moveoutput(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//player commands
char *play(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playid(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *stop(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *currentsong(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *pause(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *status(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *next(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *previous(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *repeat(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *single(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *consume(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *random(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *clearerror(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *seek(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *seekid(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *seekcur(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *crossfade(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *mixrampdb(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *mixrampdelay(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *replay_gain_mode(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *replay_gain_status(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//playlist commands
char *save(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *load(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *listplaylist(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *listplaylistinfo(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *rm(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *rename(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playlistdelete(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playlistmove(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playlistclear(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playlistadd(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *listplaylists(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//queue commands
char *add(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *addid(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *rangeid(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *delete (intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *deleteid(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playlist(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *shuffle(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *clear(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *plchanges(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *plchangesposid(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playlistinfo(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playlistid(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playlistfind(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *playlistsearch(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *prio(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *prioid(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *move(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *moveid(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *swap(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *swapid(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//sticker commands
char *sticker(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//storage commands
char *listmounts(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *mount(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *unmount(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
//tag commands
char *addtagid(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
char *cleartagid(intf_thread_t *intfa, char *arguments)
{
    char *output = malloc(sizeof(char) * 4096);
    bzero(output, 4096);
    strcat(output, "\n");
    msg_Info(intfa, "%s.", output);
    return (char *)output;
}
/*Helper Functions*/
static int string_compare_function(const void *key, const void *arrayElement)
{
    const commandFunc *commandFunc = arrayElement;
    return strcmp(key, commandFunc->command); /*each list item's string binary search*/
}
commandFunc *searchCommand(const char *key)
{
    return bsearch(key, command_list_array, ARRAY_SIZE(command_list_array), sizeof(*command_list_array), string_compare_function);
}
void trim(char *s)
{
    int i, j;

    for (i = 0; s[i] == ' ' || s[i] == '\t'; i++)
        ;

    for (j = 0; s[i]; i++)
    {
        s[j++] = s[i];
    }
    s[j] = '\0';
    for (i = 0; s[i] != '\0'; i++)
    {
        if (s[i] != ' ' && s[i] != '\t')
            j = i;
    }
    s[j + 1] = '\0';
}

/*Client Handling Thread Function*/
void *clientHandling(void *threadArg) //Polling + Client Req Handling
{
    // 0. Function Start
    intf_thread_t *intfa = (intf_thread_t *)threadArg;
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

    // 3. Event Loop
    while (true)
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
                    //medialibraryInit
                    // commandFunc* command=&command_list_array[0];
                    // char *out = command->commandName(intfa, "arg"); //correct way of recieving char* from function
                    // msg_Info(intfa,"%s",out);
                    // free(out);  //clear memory allocation.
                    // msg_Info(intfa,"1");
                    // vlc_playlist_t *playlist=intfa->p_sys->vlc_playlist;
                    // msg_Info(intfa,"2");
                    // vlc_player_t *player=vlc_playlist_GetPlayer(playlist);
                    // msg_Info(intfa,"3");
                    // input_item_t *media=input_item_New("file:///home/nightwayne/Music/IDWItTakes.mp3","wit"); //URI file path error
                    // msg_Info(intfa,"4");
                    // //playing1
                    // if(player==NULL)
                    // msg_Info(intfa,"NN");
                    // if(playlist==NULL)
                    // msg_Info(intfa,"NNds");
                    // int t;
                    // vlc_player_Lock(player);
                    // msg_Info(intfa,"5");
                    // vlc_player_SetCurrentMedia(player, media);
                    // msg_Info(intfa,"6");
                    // t=vlc_player_Start(player);
                    // msg_Info(intfa,"7",t);
                    // sleep(2);
                    // msg_Info(intfa,"8");
                    // vlc_player_Stop(player);
                    // msg_Info(intfa,"9");
                    // vlc_player_Unlock(player);
                    // msg_Info(intfa,"10");
                    // input_item_Release (media);
                    // msg_Info(intfa,"11");
                    // //playing2
                    // vlc_playlist_Lock(playlist);
                    // msg_Info(intfa,"%d",vlc_playlist_Count(playlist));
                    // vlc_playlist_Clear(playlist);
                    // msg_Info(intfa,"%d",vlc_playlist_Count(playlist));
                    // vlc_playlist_InsertOne(playlist,0,media);
                    // msg_Info(intfa,"%d",vlc_playlist_Count(playlist));
                    // vlc_playlist_Start(playlist);
                    // msg_Info(intfa,"%d",vlc_playlist_Count(playlist));
                    // vlc_playlist_Unlock(playlist);
                    // input_item_Release (media);
                    // 1. return OK connection established
                    char *output = "OK MPD Version 0.21.25";
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
                    char *input = malloc(sizeof(char) * 4096), *arguments = malloc(sizeof(char) * 4096), *command = malloc(sizeof(char) * 4096);
                    bzero(input, 4096);
                    bzero(arguments, 4096);
                    bzero(command, 4096);
                    if ((n = read(client_fd, input, 4095)) < 0)
                    {
                        perror("read");
                        exit(EXIT_FAILURE);
                    }
                    input[strlen(input) - 1] = '\0';

                    // 2. Size 0 input -handled
                    if (strlen(input) == 0)
                    {
                        //continue; ->if continued 0 length input, then needs new approach.
                        char *arguments = "arg";
                        commandFunc *command = searchCommand("listall"); //or use "commands"
                        char *output = command->commandName(intfa, arguments);
                        msg_Info(intfa, "Size0 %s", output);
                        send(client_fd, output, strlen(output), 0);
                        free(output);
                    }

                    // 3. remove whitespace in left and right ->check
                    trim(input);

                    // 4. separate command and argument using whitespace
                    arguments = strchr(input, ' ');
                    if (arguments != NULL)
                    {
                        int s = strlen(input);
                        strncat(command, input, s - strlen(arguments));
                        trim(arguments);
                    }
                    else
                        arguments = "";
                    msg_Info(intfa, "input:%s.\ninputSize:%ld.\n\narguments:%s.\nargumentsSize:%ld.\n", command, strlen(command), arguments, strlen(arguments));

                    // 5. check for lowercase - here I am explicity converting to small string
                    //tolower(command);
                    for (int i = 0; command[i]; i++)
                    {
                        command[i] = tolower(command[i]);
                    }

                    // 6. search for the command in the array
                    // commandFunc* commandF=searchCommand(input);
                    // if(commandF==NULL)
                    // {
                    //     //study the ACK.hxx & follow up
                    //     msg_Info(intfa,"command not found\n");
                    //     send(client_fd, "OK\n", strlen("OK\n"), 0);
                    //     //send(client_fd, "ACK command not found\n", strlen("ACK command not found\n"), 0);
                    //     FD_CLR(client_fd, &currentSockets); //remove client_fd to current
                    // }
                    // else
                    // {
                    //     char* output = commandF->commandName(intfa, arguments);
                    //     send(client_fd, output, strlen(output), 0);
                    //     free(output);
                    // }

                    // 7. free up dynamically allocated data
                    free(input);
                    free(command);
                    free(arguments);
                    FD_CLR(client_fd, &currentSockets); //remove client_fd to current
                }
            }
        } //for
    }     //while

    // 4. Function End
    return NULL;
}

/*Open Module*/
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

    intf_thread_t *intf = (intf_thread_t *)obj;
    intf->p_sys = p_sys;
    p_sys->server_fd = server_fd;
    p_sys->vlc_medialibrary = vlc_ml_instance_get(obj);
    p_sys->vlc_playlist = vlc_intf_GetMainPlaylist(intf);            /*vlc_intf_GetMainPlaylist(intfa);*/
    p_sys->vlc_player = vlc_playlist_GetPlayer(p_sys->vlc_playlist); /*vlc_playlist_GetPlayer(sys->vlc_playlist);*/

    // 6. Client Handling
    int i = 0;
    if ((i = vlc_clone(&intf->p_sys->thread, clientHandling, intf, VLC_THREAD_PRIORITY_LOW)) < 0)
    {
        perror("clone failed");
        exit(EXIT_FAILURE);
    }

    // 7. Module End
    return VLC_SUCCESS;
error:
    return VLC_EGENERIC;
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
