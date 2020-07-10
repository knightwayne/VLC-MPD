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

/* Internal state for an instance of the module */
// struct intf_sys_t
// {
//     int i;
//};

/*Open Module*/
static int Open(vlc_object_t *obj)
{
    // 0. Module Start 
    intf_thread_t *intf = (intf_thread_t *)obj; //thread for interface debugging
    msg_Info(intf, "Hello World! MediaLibrary Practice Started");

    // 1. Testing MediaLibrary
    //1
    int c=0;
    vlc_medialibrary_t* p_ml=vlc_ml_instance_get(obj);

    //2
    vlc_ml_query_params_t params = vlc_ml_query_params_create();
    vlc_ml_query_params_t* p_params = &params;
    //p_params->psz_pattern="Natu";
    //msg_Info(intf, "f0 %ld %ld %s",sizeof(params),sizeof(p_params),p_params->psz_pattern);
    
    // //3
    // c=vlc_ml_add_folder (p_ml, "/home/nightwayne/Music/");
    // msg_Info(intf,"f0 %d",c);

    // //4
    // void* p_get = vlc_ml_get(p_ml,VLC_ML_GET_MEDIA_BY_MRL,"/home/nightwayne/Music/Natural.mp3");
    // vlc_ml_media_t *p_mlmt = (vlc_ml_media_t*)p_get;
    // msg_Info(intf,"f0 %d",c);
    // //msg_Info(intf,"%d",p_mlmt->i_id);

    // //5
    // c = vlc_ml_control(p_ml,VLC_ML_ADD_FOLDER,"/home/nightwayne/Music/");
    // msg_Info(intf,"f0 %d",c);

    // //6
    // c = vlc_ml_list(p_ml,VLC_ML_GET_MEDIA_BY_MRL,VLC_ML_LIST_AUDIOS,p_params/* ,vlc_ml_media_list_t** */);
    // msg_Info(intf,"f0 %d",c);

    //7
    vlc_ml_media_list_t *aa =	vlc_ml_list_audio_media (p_ml, p_params);
    msg_Info(intf,"%d",aa->i_nb_items);
    vlc_ml_media_t *ans=&aa->p_items[1];
    int id=ans->i_id;
    msg_Info(intf,"%s %d",ans->psz_title,id);

    //8
    vlc_ml_artist_list_t * ab =vlc_ml_list_artists (p_ml, p_params, true);
    msg_Info(intf,"%d",ab->i_nb_items);
    vlc_ml_artist_t *anb;
    for(int i=0;i<ab->i_nb_items;i++)
    {
        anb=&ab->p_items[i];
        int id2=anb->i_id;
        msg_Info(intf,"%s %d",anb->psz_name,id2);
    }

    // pointer free error - why?
    // vlc_ml_media_release (ans);
    // msg_Info(intf,"f1 %d",c);
    // vlc_ml_media_list_release (aa);
    // msg_Info(intf,"f2 %d",c);
    // vlc_ml_artist_release (anb);
    // msg_Info(intf,"f3 %d",c);
    // vlc_ml_artist_list_release (ab);
    // msg_Info(intf,"f4 %d",c);
    
    //double free or corruption (out) Aborted (core dumped)

    // 2. Module End
    return VLC_SUCCESS;
    //error case -> add go to statements if needed
}

/*Close Module*/
static void Close(vlc_object_t *obj)
{
    // 0. Module Start
    intf_thread_t *intf = (intf_thread_t *)obj;
    
    // 1. Module End
    msg_Info(intf, "Good bye MediaLibrary!");
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