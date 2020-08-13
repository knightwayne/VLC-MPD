#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
/* VLC core API headers */
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>


//Helper Functions

// Command Functions

// commands initial
char *commands(intf_thread_t *intfa, char *arguments);
char *not_commands(intf_thread_t *intfa, char *arguments);
char *handle_update(intf_thread_t *intfa, char *arguments);

//misc
char *closeF(intf_thread_t *intfa, char *arguments);
char *ping(intf_thread_t *intfa, char *arguments);
char *password(intf_thread_t *intfa, char *arguments);
char *tagtypes(intf_thread_t *intfa, char *arguments);

//database commands
//CommandResult handle_listfiles_db(Client &client, Response &r, const char *uri----);
//CommandResult handle_lsinfo2(Client &client, const char *uri, Response &response---);
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
//CommandResult handle_listfiles_local(Response &response, Path path_fs---);
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
char *pauseF(intf_thread_t *intfa, char *arguments);
char *status(intf_thread_t *intfa, char *arguments);
char *next(intf_thread_t *intfa, char *arguments);
char *previous(intf_thread_t *intfa, char *arguments);
char *repeat(intf_thread_t *intfa, char *arguments);
char *single(intf_thread_t *intfa, char *arguments);
char *consume(intf_thread_t *intfa, char *arguments);
char *randomF(intf_thread_t *intfa, char *arguments);
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
char *renameF(intf_thread_t *intfa, char *arguments);
char *playlistdelete(intf_thread_t *intfa, char *arguments);
char *playlistmove(intf_thread_t *intfa, char *arguments);
char *playlistclear(intf_thread_t *intfa, char *arguments);
char *playlistadd(intf_thread_t *intfa, char *arguments);
char *listplaylists(intf_thread_t *intfa, char *arguments);

//queue commands
char *add(intf_thread_t *intfa, char *arguments);
char *addid(intf_thread_t *intfa, char *arguments);
char *rangeid(intf_thread_t *intfa, char *arguments);
char *deleteF (intf_thread_t *intfa, char *arguments);
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
//CommandResult handle_listfiles_storage(Response &r, Storage &storage, const char *uri--);
//CommandResult handle_listfiles_storage(Client &client, Response &r, const char *uri--);
char *listmounts(intf_thread_t *intfa, char *arguments);
char *mount(intf_thread_t *intfa, char *arguments);
char *unmount(intf_thread_t *intfa, char *arguments);

//tag commands
char *addtagid(intf_thread_t *intfa, char *arguments);
char *cleartagid(intf_thread_t *intfa, char *arguments);