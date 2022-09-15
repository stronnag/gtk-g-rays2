/*
 * Copyright (C) 2007-2013 Jonathan Hudson <jh+gps@daria.co.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "wbt201.h"
#include <libintl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

void wbt_debug(const char *format, ...)
{
    time_t t;
    char *s = NULL;
    struct tm *tm;
    char tbuf[100];
    int ret;
    va_list ap;

    va_start(ap, format);
    ret = vasprintf(&s, format, ap);
    va_end(ap);

    if (ret == -1) {
        fprintf(stderr, "vasprintf failed in _do_debug_v, cannot print debug message.\n");
        fflush(stderr);
        return;
    }
    t = time(NULL);
    tm = localtime(&t);
    strftime(tbuf,sizeof(tbuf)-1,"%F %T", tm);
    fprintf(stderr, "%s [%5u]: %s", tbuf, (unsigned)getpid(), s);
    fflush(stderr);
    free(s);
}

void kick_actions(void)
{
    char *p,*cmd,msg[64];
    if (wbt->serq)
    {
        cmd = wbt->serq->data;
        wbt->serq = g_slist_next(wbt->serq);
        if (wbt->serfd != -1)
        {
            p = stpcpy(msg, "@AL");
            if (cmd)
            {
                *p++ = ',';
                p = stpcpy(p, cmd);
            }
            p=stpcpy(p,"\n");
            *p = 0;
            
            if(wbt->verbose) wbt_debug("Init IO %s", msg);
            write_serial(msg, -1);
        }
    }
}

void set_factory_defaults(void)
{
    static char *defs[] = {"6,1,1", "6,2,1", "6,3,2000", "6,4,5",
                           "6,5,20", "6,6,60", "6,7,100", "6,9,5",
                           "6,10,10", "6,11,15", "6,12,20",
                           "6,13,5", "6,14,50", NULL };
    char **p;

    flush_xmit_queue();
    for(p=defs;*p;p++)
    {
        wbt->serq = g_slist_append(wbt->serq,*p);
    }
    kick_actions();
}

char *save_dialogue(char *name, WBT_FileFilter_t filflg, int *del)
{
    GtkWidget *dialog;
    GtkWidget *toggle = NULL, *togglet = NULL, *togglew = NULL;
    char *filename = NULL;

    dialog = gtk_file_chooser_dialog_new (_("Save File"),
                                          NULL, 
                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                          "Cancel",
                                          GTK_RESPONSE_CANCEL,
                                          "Save",
                                          GTK_RESPONSE_ACCEPT,
                                          NULL);
    
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (dialog),
                                                   TRUE);
    if(wbt->savepath)
    {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (dialog),
                                            wbt->savepath);
    }
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), name);

    GtkFileFilter *filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Any File"));
    gtk_file_filter_add_pattern (filter, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog),filter);
    filter = gtk_file_filter_new ();    
    gtk_file_filter_set_name (filter, _("GPX Files"));
    gtk_file_filter_add_pattern (filter, "*.gpx");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog),filter);
    if(filflg == WBT_FILTER_GPX)
    {
        gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog),filter);        
    }
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("KML Files"));
    gtk_file_filter_add_pattern (filter, "*.kml");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog),filter);
    if(filflg == WBT_FILTER_KML)
    {
        gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog),filter);        
    }
    gtk_file_chooser_set_do_overwrite_confirmation(
        GTK_FILE_CHOOSER (dialog),TRUE);
    if(*del)
    {
       GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL,0);
    
       if(*del & WBT_SAVE_DEL)
       {
           toggle = gtk_check_button_new_with_label(_("Delete from device"));
           gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
           gtk_widget_show (toggle);
       }
       if (*del & WBT_SAVE_NMEA)
       {
           toggle = gtk_check_button_new_with_label(_("Append if existing"));
           gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
           gtk_widget_show (toggle);
       }

       if (*del & WBT_SAVE_TRK)
       {
           togglet = gtk_check_button_new_with_label(_("Save tracks"));
           gtk_box_pack_start (GTK_BOX (vbox), togglet, FALSE, FALSE, 0);
           gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(togglet),1);
           gtk_widget_show (togglet);
       }

       if (*del & WBT_SAVE_WAY)
       {
           togglew = gtk_check_button_new_with_label("Save waypoints");
           gtk_box_pack_start (GTK_BOX (vbox), togglew, FALSE, FALSE, 0);
           gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(togglew),1);
           gtk_widget_show (togglew);
       }
       gtk_widget_show (vbox);
       gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog),
                                           vbox);
    }
    
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    }

    if(*del)
    {
        *del = 0;
        if (toggle)
        {
            if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(toggle)))
            {
                *del |= WBT_SAVE_DEL;
            }
        }
        if (togglet)
        {
            if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(togglet)))
            {
                *del |= WBT_SAVE_TRK;
            }
        }
        if (togglew)
        {
            if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(togglew)))
            {
                *del |= WBT_SAVE_WAY;
            }
        }
    }
    gtk_widget_destroy (dialog);
    return filename;
}

void switch_page(int n)
{
    
    switch(n)
    {
        case 0:
            flush_xmit_queue();
            perform_login();
            wbt->serq = g_slist_append(wbt->serq,"5,2");
            break;
        case 1:
            flush_xmit_queue();            
            wbt->serq = g_slist_append(wbt->serq,"2,1");
            break;
    }
    kick_actions();
}

void perform_login()
{
    wbt->dologin = TRUE;
    wbt->serq = g_slist_prepend(wbt->serq,NULL);
}
