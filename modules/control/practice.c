#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
/* VLC core API headers */
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_fs.h>
// #include <vlc_objects.h>
// #include <vlc_variables.h>

/* Internal state for an instance of the module */
struct intf_sys_t
{
    char *who;
};

/**
 * Starts our example interface.
 */

static int Open(vlc_object_t *obj)
{
    //---------------------------------------------------------------------User code0
    //does not work, maybe not a single file system
    //intf_thread_t *intfa = (intf_thread_t *)obj;

    // const char *oldpath = "home/nightwayne/vlc-dev/vlc/aal.c";
    // const char *newpath = "home/nightwayne/vlc-dev/vlc/aala.c";
    // int iaaa = vlc_rename(oldpath, newpath);
    // msg_Info(intfa, "Hello %d!", iaaa);
    //---------------------------------------------------------------------END User Code0

    //---------------------------------------------------------------------User code1
    // intf_thread_t *intfa = (intf_thread_t *)obj;

    // char* a = vlc_getcwd();
    // int i = vlc_mkdir("/home/nightwayne/vlc-dev/vlc/build/pear", "rwxrwxrwx");
    // msg_Info(intfa, "%d %s!", i,a);

    //---------------------------------------------------------------------END User Code1

    //---------------------------------------------------------------------User code2
    // intf_thread_t *intfa = (intf_thread_t *)obj;

    // vlc_object_t *newObj1 = vlc_object_create(obj,1000);
    // //vlc_object_t *newObj2 = vlc_object_create(obj, 100);

    // const char *p = vlc_object_typename(newObj1);
    // msg_Info(intfa, "%s", p);

    // int i1 = var_Create(newObj1, "nVar", VLC_VAR_INTEGER);
    // msg_Info(intfa, "%d", i1); //0

    // int i2 = var_Type(newObj1,"nVar");
    // msg_Info(intfa, "%d", i2); //48

    // vlc_value_t v1;
    // v1.i_int=15;
    // int i3 = var_Set(newObj1,"nVar",v1);
    // msg_Info(intfa, "%d", i3); //0

    // vlc_value_t *v2 = &v1;
    // int i4 = var_Get(newObj1, "nVar", v2);
    // msg_Info(intfa, "aa %d", i4); //aa 0

    // int i5=(*v2).i_int;
    // msg_Info(intfa, "%d", i5); //15

    // //msg_Info(intfa,"%d %d %d %d %d",i1,i2,i3,i4,i5);

    // vlc_object_delete(newObj1);
    // //vlc_object_delete(newObj2);

    //---------------------------------------------------------------------END User Code2

    //---------------------------------------------------------------------User code3

    // void *thread_func(void *arg)
    // {
    //     intf_thread_t *intfa = (intf_thread_t *)arg;
    //     char *a = vlc_getcwd();
    //     //int i = vlc_mkdir("/home/nightwayne/vlc-dev/vlc/build/pear", "rwxrwxrwx");
    //     msg_Info(intfa, "%s!", a);

    //     return NULL;
    // }

    // intf_thread_t *intfa = (intf_thread_t *)obj;
    // vlc_thread_t th;
    // int i = vlc_clone(&th, thread_func, intfa, 0);
    // msg_Info(intfa, "%d!", i);
    // vlc_join(th, NULL);

    //---------------------------------------------------------------------END User Code3

    //---------------------------------------------------------------------User code4

    // intf_sys_t *sysa = malloc(sizeof(*sys));
    // if (unlikely(sysa == NULL))
    //     return VLC_ENOMEM;
    // sysa->who = "helloHi helloHi";
    // msg_Info(intf, "HH %s", sysa->who);
    // //free(sysa->who);
    // free(sysa);

    //---------------------------------------------------------------------END User Code4
    
    intf_thread_t *intf = (intf_thread_t *)obj;

    /* Allocate internal state */
    intf_sys_t *sys = malloc(sizeof(*sys));
    if (unlikely(sys == NULL))
        return VLC_ENOMEM;
    intf->p_sys = sys; //to clear the sys in close() callback

    /* Read settings */
    char *who = var_InheritString(intf, "variable");
    if (who == NULL)
    {
        msg_Err(intf, "Nobody to say hello to!");
        goto error;
    }
    sys->who = who;
    msg_Info(intf, "Hello %s!", sys->who);
    return VLC_SUCCESS;

error:
    free(sys);
    return VLC_EGENERIC;
}

/**
 * Stops the interface. 
 */

static void Close(vlc_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj;
    intf_sys_t *sys = intf->p_sys;

    msg_Info(intf, "Good bye %s!", sys->who);

    /* Free internal state */
    free(sys->who);
    free(sys);
}

/* Module descriptor */
vlc_module_begin()
    set_shortname(N_("MPD"))
        set_description(N_("MPD Control Interface Module"))
            set_capability("interface", 0)
                set_callbacks(Open, Close)
                    set_category(CAT_INTERFACE)
                        add_string("variable", "world", "Target", "Whom to say hello to.", false) //?
    vlc_module_end()

    
