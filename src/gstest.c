#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <gio/gunixfdlist.h>

static  GMainLoop* main_loop;
static GError *error;
static int logfd;

typedef struct 
{
    gint sfd;
} spawndata_t;

static void spawn_hcitool();

static void write_error (char *message,
			 GError **error)
{
    if (error == NULL || *error == NULL) {
	return;
    }
    g_printerr ("%s: %s\n", message, (*error)->message);
    g_clear_error (error);
}


static void finalise_hcitool(GPid pid, gint status, gpointer data)
{
    char buf[1024];
    spawndata_t *s=(spawndata_t *)data;
    int n;
    char lbuf[64] = {0};

    FILE *fp = fdopen(s->sfd, "r");
        
    while(fgets(buf, sizeof(buf), fp) != NULL)
    {
        if(strstr(buf, "G-Rays2"))
        {
            sscanf(buf,"%s",lbuf);
        }
    }
    fclose(fp);
    g_spawn_close_pid(pid);
    if (*lbuf)
    {
        fprintf(stdout,"Found %s\n", lbuf);
        g_main_loop_quit(main_loop);
    }
    else
    {
        fprintf(stdout,"Retrying\n");
        spawn_hcitool();
    }
}

static void spawn_hcitool()
{
    
    static char * argv[] = {"/usr/bin/hcitool","scan",NULL};
    static spawndata_t sd;
    GPid pid;
        
    g_spawn_async_with_pipes(NULL,
                             argv,
                             NULL,
                             G_SPAWN_DO_NOT_REAP_CHILD|G_SPAWN_STDERR_TO_DEV_NULL,
                             NULL,
                             &sd,
                             &pid,
                             NULL,
                             &sd.sfd,
                             NULL,
                             NULL);
    g_child_watch_add(pid, finalise_hcitool, &sd);
}


int main(int argc, char **argv)
{
    main_loop = g_main_loop_new (NULL, FALSE);

    spawn_hcitool();
    g_main_loop_run(main_loop);
    return 0;
}

