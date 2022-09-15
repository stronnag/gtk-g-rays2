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

static void set_ui_states(int s)
{
    char *items[]={"auto_sleep1",
                   "over_speed1",
                   "low_speed_limit1",
                   "high_speed_limit1",
                   "heading_mode_int1",
                   "low_speed1",
                   "middle_speed1",
                   "high_speed1",
                   "speed_mode_time1",
                   "low_middle_int1",
                   "middle_high_int1",
                   "high_high_int1",
                   "time_mode_int1",
                   "dist_mode_int1",NULL };
    char **p;
    GtkWidget *w;
    for(p = items; *p; p++)
    {
        assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                          wbt->builder,*p))));
        gtk_entry_set_text(GTK_ENTRY(w), "");
        gtk_widget_set_sensitive(w,s);
    }
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"logcombo1"))));
    gtk_widget_set_sensitive(w,s);
}

void set_menu_states(int s)
{
    GtkWidget *w;
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"save_raw_nmea1"))));
    gtk_widget_set_sensitive(w,s);
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"save_as_kml1"))));
    gtk_widget_set_sensitive(w,s);
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"save_gpx1"))));
    gtk_widget_set_sensitive(w,s);

    /* Clear Log */
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (wbt->builder,"button1"))));
    gtk_widget_set_sensitive(w,s);
    /* Reset Defaults */
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (wbt->builder,"button2"))));
    gtk_widget_set_sensitive(w,s);
    /* Apply */
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (wbt->builder,"button4"))));
    gtk_widget_set_sensitive(w,s);
    set_ui_states(s);
}


G_MODULE_EXPORT gboolean on_draw_expose_event (GtkWidget *w, cairo_t *cr,
                                        gpointer user_data)
{
    graph_draw(w, cr);
    return FALSE;
}

G_MODULE_EXPORT void on_dev_type_changed (GtkWidget *w, gpointer user_data) {}

G_MODULE_EXPORT void on_window1_delete_event (GtkWidget *w, gpointer user_data)
{
    serial_tidy();
    gtk_main_quit();
}

G_MODULE_EXPORT void on_dialog1_delete_event (GtkWidget *w, gpointer user_data)
{
    gtk_widget_hide(w);
}


void start_serial_connect(void)
{
    char *ser;
    GtkWidget *s,*q;

    assert(NULL != (s = GTK_WIDGET (gtk_builder_get_object (
                                        wbt->builder,"conn_state1"))));
    assert(NULL != (q=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"conn_button1"))));


    gtk_image_set_from_icon_name(GTK_IMAGE(s),"edit-redo", GTK_ICON_SIZE_MENU);
    gtk_button_set_label (GTK_BUTTON(q), "Trying ...");
    gtk_widget_set_sensitive(q,FALSE);
    ser = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wbt->devcombo));
    gtk_main_iteration_do(FALSE);
    init_serial(ser, wbt->sspeed);
    g_free(ser);
}

void complete_serial_connect(void)
{
    char *ser;
    GtkWidget *s,*q;
    assert(NULL != (s = GTK_WIDGET (gtk_builder_get_object (
                                        wbt->builder,"conn_state1"))));
    assert(NULL != (q=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"conn_button1"))));

    if(wbt->verbose) wbt_debug("init returns fd,dev %d %s\n", wbt->serfd,
                               (wbt->curdev) ? wbt->curdev : wbt->btaddr);
    ser = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wbt->devcombo));
    if (wbt->serfd == -1)
    {
        if(wbt->curdev != NULL) // we found a device and failed ...
        {
            char *m,msg[256];
            const char *s= g_strerror (errno);
            m = stpcpy(msg, "Failed to create a serial connection ");
            strcpy(m, ser);
            message_box(msg,s);
        }
        gtk_button_set_label (GTK_BUTTON(q),"gtk-connect");
        gtk_widget_set_sensitive(q, TRUE);
        gtk_image_set_from_icon_name(GTK_IMAGE(s),"gtk-no",
                                     GTK_ICON_SIZE_MENU);
    }
    else
    {
        set_menu_states(TRUE);
        if(wbt->lastdev && strcmp(ser,wbt->lastdev) != 0)
        {
            g_free(wbt->lastdev);
            wbt->lastdev = g_strdup(ser);
        }
        gtk_button_set_label (GTK_BUTTON(q),"gtk-disconnect");
        gtk_widget_set_sensitive(q, TRUE);
        gtk_image_set_from_icon_name(GTK_IMAGE(s),"gtk-yes",
                                     GTK_ICON_SIZE_MENU);
        gtk_main_iteration_do(FALSE);
        flush_xmit_queue();
        start_serial_handler();
        perform_login();
        kick_actions();
    }
    g_free(ser);
}

G_MODULE_EXPORT void on_connect_clicked  (GtkWidget *w, gpointer user_data)
{
    if (wbt->serfd == -1)
    {
        start_serial_connect();
    }
    else
    {
        close_serial();
    }
}

G_MODULE_EXPORT void on_notebook_switch_page (GtkNotebook *w, void *page,
                              guint            page_num,
                              gpointer         user_data)
{
    switch_page(page_num);
}

G_MODULE_EXPORT void on_clear_clicked (GtkWidget *w, gpointer user_data)
{
    flush_xmit_queue();
    wbt->serq = g_slist_append(wbt->serq,"5,6");
    wbt->serq = g_slist_append(wbt->serq,"5,1");
    wbt->serq = g_slist_append(wbt->serq,"5,2");
    kick_actions();
}

G_MODULE_EXPORT void on_quit_clicked (GtkWidget *w, gpointer user_data)
{
    const gchar *ltext = gtk_button_get_label(GTK_BUTTON(w));

    if((0 == strcmp(ltext,"gtk-quit")) || (0 == strcmp(ltext,"Quit")))
    {
        serial_tidy();
        saveprefs();
        gtk_main_quit();
    }
    else
    {
        if(wbt->verbose) wbt_debug("Hit %s\n", ltext);
        if(wbt->g)
        {
                // FIXME

        }
        else
        {
            reset_quit_status();
            complete_serial_connect();
        }
    }
}

G_MODULE_EXPORT void on_quit1_activate (GtkWidget *w, gpointer user_data)
{
    serial_tidy();
    saveprefs();
    gtk_main_quit();
}

G_MODULE_EXPORT void on_about1_activate (GtkWidget *w, gpointer user_data)
{
    GtkWidget *q;
    assert(NULL != (q=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"about"))));
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(q),VERSION);
    gtk_widget_show(q);
}


G_MODULE_EXPORT void on_log_type_changed (GtkWidget *w, gpointer user_data)
{
    int val = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
    set_edit_state(val);
}

G_MODULE_EXPORT void on_prefs_activate (GtkWidget *w, gpointer user_data)
{
    GtkWidget *q,*tv,*sb,*kb,*sp, *ug;
    GtkTextBuffer *tb;
    GtkTextIter is,ie;
    char **d;
    char *text0, *text1;
    int n;

    assert(NULL != (q=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"dialog1"))));
    assert(NULL != (tv=GTK_WIDGET (gtk_builder_get_object (
                                       wbt->builder,"devnames"))));
    assert(NULL != (sb=GTK_WIDGET (gtk_builder_get_object (
                                       wbt->builder,"rembutton"))));
    assert(NULL != (kb=GTK_WIDGET (gtk_builder_get_object (
                                       wbt->builder,"kmlmarks"))));
    assert(NULL != (ug=GTK_WIDGET (gtk_builder_get_object (
                                       wbt->builder,"use_dev_gps"))));
    assert(NULL != (sp=GTK_WIDGET (gtk_builder_get_object (
                                       wbt->builder,"savedirbutton"))));

    if(wbt->lastdev)
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(sb),1);
    }

    if(wbt->savepath)
    {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(sp),
                                            wbt->savepath);
    }

    if(wbt->usedevgps)
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(ug),1);
    }

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(kb),wbt->trackmarks);
    tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_text_buffer_get_start_iter (tb, &is);
    gtk_text_buffer_get_end_iter (tb, &ie);
    gtk_text_buffer_delete (tb, &is, &ie);
    for(d = wbt->devs; (d && *d); d++)
    {
        if(**d && !g_ascii_isspace (**d))
        {
            gtk_text_buffer_insert (tb,&ie,*d,-1);
            gtk_text_buffer_insert (tb,&ie,"\n",1);
        }
    }
    gtk_text_buffer_get_start_iter (tb, &is);
    text0 = gtk_text_buffer_get_text(tb,&is,&ie,TRUE);
    gtk_widget_show(q);
    n =  gtk_dialog_run(GTK_DIALOG(q));
    gtk_widget_hide(q);
    if(n == GTK_RESPONSE_OK)
    {
        wbt->trackmarks = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(kb));
        wbt->usedevgps = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(ug));

        if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(sb)) == TRUE)
        {
            char *resp;
            resp = gtk_combo_box_text_get_active_text(
                GTK_COMBO_BOX_TEXT(wbt->devcombo));
            if(wbt->lastdev)
            {
                if(strcmp(wbt->lastdev,resp) != 0)
                {
                    g_free(wbt->lastdev);
                    wbt->lastdev = g_strdup(resp);
                }
            }
            else
            {
                wbt->lastdev = g_strdup(resp);
            }

            resp = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(sp));

            if(wbt->savepath)
            {
                if(strcmp(wbt->savepath,resp) != 0)
                {
                    g_free(wbt->savepath);
                    wbt->savepath = g_strdup(resp);
                }
            }
            else
            {
                wbt->savepath = g_strdup(resp);
            }

        }
        else
        {
            g_free(wbt->lastdev);
            wbt->lastdev = NULL;
        }
        gtk_text_buffer_get_start_iter (tb, &is);
        gtk_text_buffer_get_end_iter (tb, &ie);
        text1 = gtk_text_buffer_get_text(tb,&is,&ie,TRUE);
        if(strcmp(text0, text1))
        {
            g_strfreev(wbt->devs);
            wbt->devs = g_strsplit_set(text1," \n\r\t", -1);
            makecombo();
        }
        g_free(text0);
        g_free(text1);
        saveprefs();
    }
}

static void stamp_fn(char *basefn, size_t basesz, char *fmt)
{
    struct tm tm;
    time_t t;
    t = time(NULL);
    localtime_r(&t, &tm);
    strftime(basefn, basesz, fmt, &tm);
}

G_MODULE_EXPORT void on_save_raw_nmea1_activate (GtkWidget *w, gpointer user_data)
{
    if(wbt->nmea_fp)
    {
        fclose(wbt->nmea_fp);
        wbt->nmea_fp = NULL;
    }
    else if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
    {
        int del=WBT_SAVE_NMEA;
        char *fn;
        char basefn[128];

        stamp_fn(basefn, sizeof(basefn), "nmea_%FT%H.%M.%S.txt");

        fn = save_dialogue(basefn, WBT_FILTER_NONE, &del);
        if(fn)
        {
            wbt->nmea_fp = fopen(fn,(del) ? "a" : "w");
        }
        else
        {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w),FALSE);
        }
    }
}

G_MODULE_EXPORT void  on_set_clicked (GtkWidget *w, gpointer user_data)
{
    do_set_actions();
}

G_MODULE_EXPORT void on_defaults_clicked (GtkWidget *w, gpointer user_data)
{
    set_factory_defaults();
}

G_MODULE_EXPORT void on_save_gpx1_activate (GtkWidget *w, gpointer user_data)
{
    int del=(WBT_SAVE_DEL|WBT_SAVE_TRK|WBT_SAVE_WAY);
    char *fn;
    char basefn[128];

    if(check_babel_version() == 1)
    {
        stamp_fn(basefn, sizeof(basefn), "track_%FT%H.%M.%S.gpx");
        fn = save_dialogue(basefn, WBT_FILTER_GPX, &del);
        if(fn)
        {
            if (del & (WBT_SAVE_DEL|WBT_SAVE_TRK|WBT_SAVE_WAY))
                babel_spawn(fn, "gpx",del);
            g_free(fn);
        }
    }
}

G_MODULE_EXPORT void on_save_as_kml1_activate (GtkWidget *w, gpointer user_data)
{
    int del=(WBT_SAVE_DEL|WBT_SAVE_TRK|WBT_SAVE_WAY);
    char *fn;
    char basefn[128];

    if(check_babel_version() == 1)
    {
        stamp_fn(basefn, sizeof(basefn), "track_%FT%H.%M.%S.kml");
        fn = save_dialogue(basefn, WBT_FILTER_GPX, &del);
        if(fn)
        {
            if (del & (WBT_SAVE_DEL|WBT_SAVE_TRK|WBT_SAVE_WAY))
            {
                char cmd[32] = "kml";
                if(wbt->trackmarks == FALSE)
                {
                    strcat(cmd,",points=0");
                }
                babel_spawn(fn, cmd, del);
            }
            g_free(fn);
        }
    }
}

G_MODULE_EXPORT void on_about_close (GtkWidget *w, gpointer user_data)
{
    gtk_widget_hide(w);
}

G_MODULE_EXPORT void on_show_help (GtkWidget *w, gpointer user_data)
{
    char path[PATH_MAX];
    strcpy(path, "yelp " HELPDIR "/index.page");
    if(FALSE == g_spawn_command_line_async(path, NULL)) {
      message_box("spawning help",
                  "please see " HELPDIR "/index.page");
    }
}
