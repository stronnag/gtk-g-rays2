/* -*- mode: C; coding: utf-8; -*-
 */

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
#include <glib/gstdio.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <libintl.h>
#include <fcntl.h>
#include <errno.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <locale.h>

#ifdef __linux
#define DEFDEVS "[g-rays2]\ndevices = auto;bluetooth;/dev/ttyUSB0\n"
#elif defined(__FreeBSD__)
#define DEFDEVS "[g-rays2]\ndevices =/dev/cuaU0\n"
#else
#define DEFDEVS "[g-rays2]\ndevices = (undefined)\n"
#endif

#include "wbt201.h"

#ifndef PACKAGE_DATA_DIR
# define PACKAGE_DATA_DIR "."
#endif

#ifndef PACKAGE_LOCALE_DIR
# define PACKAGE_LOCALE_DIR "."
#endif

wbt201_t *wbt;

void message_box(const char *op, const char *s)
{
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     _("Error %s : %s"),
                                     op, s);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

void saveprefs(void)
{
    int fd;
    char **d;
    GString *g;
    g = g_string_new("[g-rays2]\ndevices=");


    g_mkdir_with_parents(wbt->confdir,0700);
    fd = g_open(wbt->confnam, O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if(fd != -1)
    {
        for(d = wbt->devs; (d && *d); d++)
        {
            if(**d && !g_ascii_isspace (**d))
            {
                g_string_append(g, *d);
                g_string_append_c(g,';');
            }
        }
        *(g->str+g->len-1) = '\n';

        if(wbt->lastdev)
        {
            g_string_append(g, "lastdev=");
            g_string_append(g, wbt->lastdev);
            g_string_append_c(g,'\n');
        }

        if(wbt->usedevgps)
        {
            g_string_append(g, "usedevgps=true\n");
        }

        if(wbt->savepath)
        {
            g_string_append(g, "savepath=");
            g_string_append(g, wbt->savepath);
            g_string_append_c(g,'\n');
        }

        if(wbt->trackmarks)
        {
            g_string_append(g, "trackmarks=true\n");
        }

        if(wbt->defbtaddr)
        {
            g_string_append(g, "btaddr=");
            g_string_append(g, wbt->defbtaddr);
            g_string_append_c(g,'\n');
        }
        else if(wbt->btaddr)
        {
            g_string_append(g, "btaddr=");
            g_string_append(g, wbt->btaddr);
            g_string_append_c(g,'\n');
        }

        if (write(fd,g->str,g->len))
            ;  /* fail, silently to shutup older gcc */
        close(fd);
        g_string_free(g,TRUE);
    }
    else
    {
        const char *s= g_strerror (errno);
        message_box(_("writing preferences"),s);
    }
}

static void getprefs(void)
{
    gboolean res = FALSE;
    GKeyFile *kf = g_key_file_new();
    gsize len;

    if(wbt->confdir == NULL)
        wbt->confdir = g_build_filename(g_get_user_config_dir(), "g-rays2", NULL);
    if(wbt->confnam == NULL)
        wbt->confnam = g_build_filename(wbt->confdir, "g-rays2rc",NULL);

    if(wbt->confnam)
    {
        res = g_key_file_load_from_file(kf, wbt->confnam, G_KEY_FILE_NONE, NULL);
    }

    if (res == FALSE)
    {
        g_key_file_load_from_data(kf, DEFDEVS, sizeof(DEFDEVS)-1,
                                  G_KEY_FILE_NONE, NULL);
    }

    wbt->devs = g_key_file_get_string_list(kf,"g-rays2","devices",
                                                 &len, NULL);
    wbt->lastdev =  g_key_file_get_string(kf,"g-rays2","lastdev", NULL);
    wbt->usedevgps =  g_key_file_get_boolean(kf,"g-rays2","usedevgps", NULL);
    wbt->savepath = g_key_file_get_string(kf,"g-rays2","savepath", NULL);
    wbt->trackmarks = g_key_file_get_boolean(kf,"g-rays2","trackmarks", NULL);
    wbt->defbtaddr = g_key_file_get_string(kf,"g-rays2","btaddr", NULL);
    wbt->defrfcom  = g_key_file_get_string(kf,"g-rays2","rfcom", NULL);

    if(res == FALSE)
    {
        saveprefs();
    }
}

char *find_gps_dev(void)
{
    int n;
    static char gdev[]="/dev/gpsX";
    char *rgdev=NULL;

    for(n = 0; n < 10; n++)
    {
        gdev[8] = '0' + n;
        if(access(gdev, R_OK|W_OK) == 0)
        {
            rgdev = gdev;
            break;
        }
    }
    return rgdev;
}


void makecombo(void)
{
    GtkWidget *w;
    char **d;
    int lastn = 0;
    int n;
    if(wbt->devcombo == NULL)
    {
        assert(NULL != (w = GTK_WIDGET (gtk_builder_get_object (wbt->builder, "table1"))));
        wbt->devcombo = gtk_combo_box_text_new();
        gtk_grid_attach(GTK_GRID(w),wbt->devcombo,1,0,1,1);
        gtk_widget_show(wbt->devcombo);
    }
    else
    {
        for(n = wbt->ndevs-1; n >= 0; n--)
        {
            gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT(wbt->devcombo),n);
        }
    }

    for(n = 0, d = wbt->devs; (d && *d); d++)
    {
        if(**d && !g_ascii_isspace (**d))
        {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wbt->devcombo),*d);
            if(wbt->lastdev && 0 == strcmp(*d, wbt->lastdev))
            {
                lastn = n;
            }
            n++;
        }
    }


    if (wbt->usedevgps)
    {
        int ndev=-1;
        int dn;

        char *gdev = find_gps_dev();
        if (gdev)
        {
            for(dn = 0, d = wbt->devs; (d && *d); d++)
            {
                if(**d && !g_ascii_isspace (**d))
                {
                    if(0 == strcmp(*d, gdev))
                    {
                        ndev = dn;
                        break;
                    }
                    dn++;
                }
            }
            if(ndev != -1)
            {
                lastn = ndev;
            }
            else
            {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wbt->devcombo),gdev);
                lastn = n;
                n++;
            }
        }
    }
    wbt->ndevs = n;
    gtk_combo_box_set_active(GTK_COMBO_BOX(wbt->devcombo), lastn);
}

gboolean Accel(GtkAccelGroup *accel_group, GObject *acceleratable,
               guint keyval, GdkModifierType modifier)
{
    if (wbt->serfd > 0)
    {
        GtkWidget *w;
        assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (wbt->builder,
                                                              "notebook1"))));
        if(gtk_notebook_get_current_page (GTK_NOTEBOOK(w)) == 1)
        {
            const gchar *p;

            assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (wbt->builder,
                                                                  "utc"))));
            p = gtk_entry_get_text(GTK_ENTRY(w));
            if(p && strlen(p))
            {
                char buf[512];
                char *s;
                s = stpcpy(buf, p);
                *s++ = '|';
                assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                                  wbt->builder,"lat"))));
                p = gtk_entry_get_text(GTK_ENTRY(w));
                s = stpcpy(s, p);
                *s++ = ' ';
                assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                                  wbt->builder,"lon"))));
                p = gtk_entry_get_text(GTK_ENTRY(w));
                s = stpcpy(s, p);
                *s++ = '|';
                assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                                  wbt->builder,"alt"))));
                p = gtk_entry_get_text(GTK_ENTRY(w));
                s = stpcpy(s, p);
                double rms;
                rms = sqrt(((wbt->gps.hdop*wbt->gps.hdop) +
                            (wbt->gps.vdop*wbt->gps.vdop) +
                            (wbt->gps.pdop*wbt->gps.pdop))/3.0);
                sprintf(s,"|%.1f\n", rms);
                gtk_clipboard_set_text(
                    gtk_clipboard_get(GDK_SELECTION_PRIMARY),
                    buf, strlen(buf));
                gtk_clipboard_store(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
            }
        }
    }
    return TRUE;
}


static void make_accel(void)
{
    GtkWidget *w;
    GtkAccelGroup *accel = gtk_accel_group_new();
    GClosure *closure;
    closure = g_cclosure_new(G_CALLBACK(Accel),NULL,NULL);
    gtk_accel_group_connect(accel,GDK_KEY_C,GDK_CONTROL_MASK, 0, closure);
    assert(NULL != (w = GTK_WIDGET (gtk_builder_get_object (
                                        wbt->builder, "window1"))));
    gtk_window_add_accel_group(GTK_WINDOW(w),accel);
}

static void setup(void)
{
    GtkWidget *w;
    GtkTextIter is;

    set_menu_states(FALSE);
    getprefs();
    makecombo();
    assert(NULL != (w = GTK_WIDGET (gtk_builder_get_object (
                                        wbt->builder, "logcombo1"))));
    gtk_combo_box_set_active(GTK_COMBO_BOX(w), 0);
    wbt->serfd = -1;
    assert(NULL != (wbt->tv= GTK_WIDGET (gtk_builder_get_object (
                                             wbt->builder,"gps_text"))));
    wbt->tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wbt->tv));
    gtk_text_buffer_get_start_iter (wbt->tb, &is);
    wbt->eob = gtk_text_buffer_create_mark(wbt->tb, "eob", &is, FALSE);
    wbt->serq = NULL;
    make_accel();
}

int main (int argc, char **argv)
{
    gboolean show_vers = FALSE;
    gboolean verbose=FALSE;

    GError *error = NULL;
    GOptionContext *context;
    GOptionEntry entries[] = {
        { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, _("be verbose"), NULL },
        { "version", 'V', 0, G_OPTION_ARG_NONE, &show_vers,
          _("version info"), NULL },
        { NULL }
    };

    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    context = g_option_context_new (_("- configure wbt-201 GPS"));
    g_option_context_set_ignore_unknown_options(context,FALSE);
    g_option_context_add_main_entries (context, entries,NULL);
    if(g_option_context_parse (context, &argc, &argv,&error) == FALSE)
    {        if (error != NULL)        {
            fprintf (stderr, "%s\n", error->message);
            g_error_free (error);
            exit(1);
        }
    }

    if(show_vers)
    {
        fputs(PACKAGE " v" VERSION "\n", stderr);
        exit(0);
    }

    wbt = g_malloc0(sizeof(*wbt));
    wbt->verbose = verbose;
    setlocale (LC_ALL, "");
    gtk_init (&argc, &argv);

    wbt->builder = gtk_builder_new ();
    if (!gtk_builder_add_from_file (wbt->builder,
                                    PACKAGE_DATA_DIR "/" "gtk-g-rays2.ui",
                                    &error))
    {
        g_warning ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
        return 0;
    }
   gtk_builder_connect_signals (wbt->builder, NULL);
   setup();
   setup_signals();
   gtk_main ();

   return 0;
}
