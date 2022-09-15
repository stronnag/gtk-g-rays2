#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "wbt201.h"
#include <libintl.h>
#include <sys/wait.h>
#include <signal.h>

void childexit(int sig)
{
    int sts;
    wait(&sts);
    wbt->cpid = -1;
}

gboolean ossilate(gpointer d)
{
    GtkWidget **w = (GtkWidget **)d;    
    if(wbt->cpid == -1)
    {
        gtk_dialog_response (GTK_DIALOG(w[0]),GTK_RESPONSE_DELETE_EVENT);
        return FALSE;
    }
    else
    {
        gtk_progress_bar_pulse (GTK_PROGRESS_BAR (w[1]));
    }
    return TRUE;
}

int check_babel_version(void)
{
    FILE *fp;
    int babel_ok=0;
    fp = popen("gpsbabel -V", "r");
    if(fp)
    {
        char bab[256];
        int digits[4];
        
        while(fgets(bab,sizeof(bab), fp))
        {
            int n=sscanf(bab,"GPSBabel Version %d.%d.%d",
                     digits,digits+1,digits+2);

            if(n == 3)
            {
                int vers = digits[2]+10*(digits[1]+10*digits[0]);
                if(vers >= 134)
                {
                    babel_ok = 1;
                }
                break;
            }
        }
        pclose(fp);
    }
    
    if(0 == babel_ok)
    {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         _("GPSbabel version is too old"));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
    }
    return babel_ok;
}

void childish_dialogue()
{
    GtkWidget *w[2];
    GtkWidget *q;
    int n;
   
    w[0] = gtk_dialog_new_with_buttons (
        _("Gpsbabel download"),
        GTK_WINDOW(GTK_WIDGET (gtk_builder_get_object(
                                   wbt->builder,"window1"))),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", 42,
        NULL);

    w[1] = gtk_progress_bar_new ();

    gtk_dialog_add_action_widget(GTK_DIALOG(w[0]), w[1], 0);
    gtk_widget_show_all (w[0]);
    g_timeout_add (100, ossilate, w);
    gint result = gtk_dialog_run (GTK_DIALOG (w[0]));
    switch (result)
    {
        case 42:
            if(wbt->cpid != -1)
                kill(wbt->cpid, SIGTERM);
            break;
        default:
            break;
    }
    gtk_widget_destroy (w[1]);
    gtk_widget_destroy (w[0]);
    
    fcntl(wbt->serfd, F_SETFL, fcntl(wbt->serfd,F_GETFL)|O_NONBLOCK);
    setup_serial(wbt->serfd, wbt->sspeed);
    wbt->tag = g_io_add_watch (wbt->gio, G_IO_IN|G_IO_HUP|G_IO_ERR|G_IO_NVAL, read_data, NULL);
    assert(NULL != (q=GTK_WIDGET (gtk_builder_get_object(
                                      wbt->builder,"notebook1"))));
    n = gtk_notebook_get_current_page(GTK_NOTEBOOK(q));
    switch_page(n);
}

static void babel_init()
{
    static int init = 0;
    if(init == 0)
    {
        struct sigaction sac;
        sigemptyset(&(sac.sa_mask));
        sac.sa_flags=0;
        sac.sa_handler=childexit;
        sigaction(SIGCHLD, &sac, NULL);  
        sigaction(SIGPIPE, &sac, NULL);  
        init = 1;
    }
}

void babel_spawn(char *fn, char* type, int del)
{
    pid_t cpid;
    char act[64] = "wbt";
    char *args[32] = {"gpsbabel"};
    int n = 1;

    babel_init();

    if(del & WBT_SAVE_DEL)
    {
        strcat(act,",erase");
    }
    if (del & WBT_SAVE_TRK)
    {
        args[n] = "-t"; n++;
    }
    if (del & WBT_SAVE_WAY)
    {
        args[n] = "-w"; n++;
    }
    args[n] = "-i"; n++;
    args[n] = act; n++;
    args[n] = "-o"; n++;
    args[n] = type; n++;
    args[n] = "-f-"; n++;
    args[n] = "-F"; n++;
    args[n] = fn;  n++;
    args[n] = NULL;

#if 0
    if(wbt->verbose)
    {
        int j;
        for(j = 0; j < n; j++)
        {
            fputs(args[j],stderr);
            fputc(' ',stderr);
        }
        fputc('\n',stderr);
    }
#endif
    
    g_source_remove (wbt->tag);
    
    switch((cpid = fork()))
    {
        case 0:
            dup2(wbt->serfd, 0);
            fcntl(0, F_SETFL, fcntl(0,F_GETFL)&~O_NONBLOCK);
            execvp (args[0], args);
            _exit(0);
            break;
        case -1:
            _exit(EXIT_FAILURE);
            break;
        default:
            wbt->cpid = cpid;
            childish_dialogue();
            break;
    }
}
