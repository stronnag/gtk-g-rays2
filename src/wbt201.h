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

#ifndef __wbt201_h
#define __wbt201_h 1

#include <stdio.h>
#include "version.h"
#include <glib/gi18n.h>
#include "bluebus.h"
#include <cairo.h>

typedef struct
{
    short prn;
    short elev;
    short azimuth;
    short snr;
    short in_use;
} sat_info_t;

typedef struct
{
    int log_type;
    char device_name[32];
    char device_info[32];
    char hw_version[16];
    char sw_version[16];
    char log_version[16];
    int  log_start;
    int  log_end;
    int wsvals[3];
    int wvvals[15];
} cfg_info_t;

typedef struct
{
    time_t utc;
    double lat;
    double lon;
    double var;
    double course;
    double speed;
    double alt;
    double height;
    int usec;
    int nsat;
    int nview;
    double hdop;
    double vdop;
    double pdop;
    short prn[32];
    sat_info_t sinfo[32];
    char alat[32];
    char alon[32];
    char valid_rmc;
    char status_gga;
    char mode1_gsa;
    char mode2_gsa;
    char state;
} gps_info_t;

typedef struct
{
    GtkBuilder *builder;
    char *serbuf;
    int serfd;
    GIOChannel *gio;
    guint tag;
    GtkWidget *devcombo;
    GtkWidget *tv;
    GtkTextBuffer *tb;
    GtkTextMark *eob;
    gps_info_t gps;
    cfg_info_t cfg;
    GSList *serq;
    short connstat;
    pid_t cpid;
    FILE *nmea_fp;
    char *confdir;
    char *confnam;
    char **devs;
    char *lastdev;
    int ndevs;
    char *curdev;
    char *savepath;
    gboolean verbose;
    gboolean trackmarks;
    gboolean usedevgps;
    gboolean dologin;
    G_rays *g;
    int sfd;
    int sspeed;
    char * defbtaddr;
    char *defrfcom;
    char *btaddr;
    int fd;
} wbt201_t;

typedef enum {
     WBT_FILTER_NONE,
     WBT_FILTER_GPX,
     WBT_FILTER_KML,
 } WBT_FileFilter_t;

#define WBT_SAVE_DEL  1
#define WBT_SAVE_NMEA 2
#define WBT_SAVE_TRK  4
#define WBT_SAVE_WAY  8

extern wbt201_t *wbt;
extern void init_serial (char *, int);
extern void serial_process(gchar *);
extern void  kick_actions(void);
extern void  do_set_actions(void);
extern void set_edit_state(int);
extern void set_factory_defaults(void);
extern char *save_dialogue(char *, WBT_FileFilter_t, int *);
extern void babel_spawn(char *, char*, int);
extern gboolean read_data(GIOChannel *source, GIOCondition condition,
                          gpointer data);
extern void setup_serial(int,int);
extern void close_serial(void);
extern void switch_page(int);
extern void set_menu_states(int);
extern void saveprefs(void);
extern void makecombo(void);
extern void message_box(const char *, const char *);
extern int check_babel_version(void);
extern void serial_tidy(void);
extern void complete_serial_connect(void);
extern void start_serial_connect(void);
extern void setup_signals(void);
extern void clean_bluez(void);
extern void reset_quit_status(void);
extern void done_serial(void);
extern void stop_discovery(G_rays*);
extern void flush_xmit_queue(void);
extern void wbt_debug(const char *,...);
extern void write_serial(char *msg, int len);
extern void start_serial_handler();
extern void perform_login();

extern void graph_draw(GtkWidget *,cairo_t *);

#endif
