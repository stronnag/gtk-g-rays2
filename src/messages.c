#include <gtk/gtk.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <libintl.h>
#include <time.h>
#include "wbt201.h"

#include <locale.h>

static char *wmodes[] = {
    /*0,1*/	NULL,NULL,
    /*2,3*/	"low_speed_limit1", "high_speed_limit1",
    /*4,5*/	"heading_mode_int1", "low_speed1",
    /*6,7*/	"middle_speed1", "high_speed1",
    /*8,9*/	NULL, "speed_mode_time1",
    /*10,11*/	"low_middle_int1", "middle_high_int1",
    /*12,13*/	"high_high_int1", "time_mode_int1",
    /*14*/	"dist_mode_int1"
};
#define WMODES_LEN (sizeof(wmodes)/sizeof(char*))

static char *wsets[] = {NULL, "auto_sleep1", "over_speed1"};
#define WSETS_LEN (sizeof(wsets)/sizeof(char*))

static short logedits[][10] = {
    {},
    {2,3,4},
    {2,3,5,6,7,9,10,11,12},
    {2,3,13},
    {2,3,14},
    {2,3,13,14}
};


void flush_xmit_queue(void)
{
    g_slist_free (wbt->serq);
    wbt->serq = NULL;
}

static inline double strtod_nonlocale(const char *buffer, char **endptr)
{
    double result;
    setlocale(LC_NUMERIC, "C");
    result = strtod(buffer, endptr);
    setlocale(LC_NUMERIC, "");
    return result;
}

gboolean is_changed(char **name, short j, int old, int *pnv)
{
    GtkWidget *w;
    const gchar *v;
    int nv;
    gboolean res = FALSE;

    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder, name[j]))));
    v= gtk_entry_get_text(GTK_ENTRY(w));
    nv = strtol(v, NULL, 10);
    if (nv != old)
    {
        res = TRUE;
        *pnv = nv;
    }
    return res;
}


void do_set_actions(void)
{
    GtkWidget *w;
    short i;
    int newval;

    char cmd[512]; /* just to avoid mem-mgmt on the list */
    int n = 0;

    flush_xmit_queue();
    for(i = 0; i < WSETS_LEN;i++)
    {
        if(wsets[i] && is_changed(wsets, i, wbt->cfg.wsvals[i], &newval))
        {
            sprintf(cmd,"1,%d,%d", i, newval);
            wbt->serq = g_slist_append(wbt->serq, cmd + n*16);
            n++;
        }
    }

    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder, "logcombo1"))));
    if (wbt->cfg.wvvals[1] != (newval = gtk_combo_box_get_active(GTK_COMBO_BOX(w))))
    {
        sprintf(cmd,"6,1,%d", newval);
        wbt->serq = g_slist_append(wbt->serq, cmd + n*16);
        n++;
    }

    for(i = 0; i < WMODES_LEN;i++)
    {
        if(wmodes[i] && is_changed(wmodes, i, wbt->cfg.wvvals[i], &newval))
        {
            sprintf(cmd,"6,%d,%d", i, newval);
            wbt->serq = g_slist_append(wbt->serq, cmd + n*16);
            n++;
        }
    }
    kick_actions();
}

void set_edit_state(int logval)
{
    GtkWidget *w;
    short i,*logp;

    for(i = 0; i < WMODES_LEN;i++)
    {
        if(wmodes[i])
        {
            assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                              wbt->builder,wmodes[i]))));
            gtk_widget_set_sensitive(w,FALSE);
        }
    }
    logp = logedits[logval];
    while((i = *logp++))
    {
        assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                          wbt->builder, wmodes[i]))));
        gtk_widget_set_sensitive(w,TRUE);
    }
}

static void nmea_display()
{
    static char* valid[] = {"Valid","Invalid"};
    static char* fix_qual[]={"","GPS","DGPS"};
    static char* gsa_mode1[]={"manaul","automatic"};
    static char* gsa_mode2[]={NULL,"no","2D", "3D"};

    GtkWidget *w;
    char tbuf[64];
    size_t n;

    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"status"))));
    snprintf(tbuf, sizeof(tbuf),_("%s %s %s %s fix"),
             _(valid[(int)wbt->gps.valid_rmc]),
             fix_qual[(int)wbt->gps.status_gga],
             _(gsa_mode1[(int)wbt->gps.mode1_gsa]),
             _(gsa_mode2[(int)wbt->gps.mode2_gsa]));

    gtk_entry_set_text(GTK_ENTRY(w), tbuf);

    n = strftime(tbuf, sizeof(tbuf), "%FT%T%z", localtime(&wbt->gps.utc));
    *(tbuf+n) = 0;
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"utc"))));
    gtk_entry_set_text(GTK_ENTRY(w), tbuf);
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"lat"))));
    gtk_entry_set_text(GTK_ENTRY(w), wbt->gps.alat);
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"lon"))));
    gtk_entry_set_text(GTK_ENTRY(w), wbt->gps.alon);
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"alt"))));
    sprintf(tbuf, "%.1f m", wbt->gps.alt);
    gtk_entry_set_text(GTK_ENTRY(w), tbuf);
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"speed"))));
    sprintf(tbuf, "%.1f knots", wbt->gps.speed);
    gtk_entry_set_text(GTK_ENTRY(w), tbuf);
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"course"))));
    sprintf(tbuf, "%.1f T", wbt->gps.course);
    gtk_entry_set_text(GTK_ENTRY(w), tbuf);
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"tiff"))));
    sprintf(tbuf, _("Satellites: %d/%d H/V/PDOP: %.1f/%.1f/%.1f"),
            wbt->gps.nsat,wbt->gps.nview, wbt->gps.hdop,
            wbt->gps.vdop, wbt->gps.pdop);
    gtk_entry_set_text(GTK_ENTRY(w), tbuf);
    assert(NULL != (w=GTK_WIDGET (gtk_builder_get_object (
                                      wbt->builder,"drawingarea1"))));
    gtk_widget_queue_draw(w);
}


static double decpos(char *pos, char *sign, char *apos)
{
    int d,im;
    double dd,dm,ds;
    char *fmt = NULL,*p=pos;


    d =  (*p - '0')*10 + (*(p+1) - '0');
    p += 2;
    if (*sign == 'N' || *sign == 'S')
    {
        fmt = "%02d:%02d:%05.2f%c";
    }
    else
    {
        fmt = "%03d:%02d:%05.2f%c";
        d = d*10 + (*p - '0');
        p++;
    }
    dm = strtod_nonlocale(p, NULL);
    im = (int)dm;
    ds = 60.0*(dm-im);
    sprintf(apos,fmt,d,im,ds,*sign);
    dd = d + dm/60.0;
    if(*sign == 'S' || *sign == 'W')
        dd = -dd;


    return dd;
}

static void process_rmc(char *msg)
{
    struct tm tm ={0};
    char temp[32];
    int j;
    char *r,*s = msg;

    for(j =0; (r = strpbrk(s,",*\n")) && j < 32; j++)
    {
        int n = (int)(r-s);
        switch (j)
        {
            case 1:
                tm.tm_hour = (s[0]-'0')*10 + (s[1]-'0');
                tm.tm_min = (s[2]-'0')*10 + (s[3]-'0');
                tm.tm_sec = (s[4]-'0')*10 + (s[5]-'0');
                if (n > 6 && s[6] == '.')
                {
                    wbt->gps.usec = strtol(s+7,NULL,10);
                }
                break;
            case 2:
                wbt->gps.valid_rmc = *s == 'A' ? 0 : 1;
                break;
            case 3:
                    // FIXME
                memcpy(temp, s, n);
                *(temp+n) = 0;
                break;
            case 4:
                wbt->gps.lat = decpos(temp,s,wbt->gps.alat);
            case 5:
                    // FIXME
                memcpy(temp, s, n);
                *(temp+n) = 0;
                break;
            case 6:
                wbt->gps.lon = decpos(temp,s,wbt->gps.alon);
                break;
            case 7:
                wbt->gps.speed = strtod_nonlocale(s,NULL);
                break;
            case 8:
                wbt->gps.course = strtod_nonlocale(s,NULL);
                break;
            case 9:
                tm.tm_mday = (s[0]-'0')*10 + (s[1]-'0');
                tm.tm_mon = (s[2]-'0')*10 + (s[3]-'0') -1;
                tm.tm_year = (s[4]-'0')*10 + (s[5]-'0') + 100;
                wbt->gps.utc = timegm(&tm);
                break;
            case 10:
                wbt->gps.var = strtod_nonlocale(s,NULL);
                break;
            case 11:
                if (*s == 'E')
                    wbt->gps.var = -wbt->gps.var;
                break;
        }
        s = ++r;
    }
    if (j) wbt->gps.state=0;
}

static void process_gga(char *msg)
{
    int j;
    char *r,*s = msg;

    for(j =0; (r = strpbrk(s,",*\n")) && j < 32; j++)
    {
        switch(j)
        {
            case 6:
                wbt->gps.status_gga = (*s - '0');
                break;
            case 7:
                wbt->gps.nsat = strtol(s,NULL,10);
                break;
            case 8:
                wbt->gps.hdop = strtod_nonlocale(s,NULL);
                break;
            case 9:
                wbt->gps.alt = strtod_nonlocale(s,NULL);
            case 10:
                    // if (*s != 'M') then what
                break;
            case 11:
                wbt->gps.height = strtod_nonlocale(s,NULL);
                break;
            case 12:
                    // if (*s != 'M') then what
                break;
        }
        s = ++r;
    }
    if (j)
        wbt->gps.state++;
}

static void process_gsa(char *msg)
{
    int j;
    char *r,*s = msg;

    for(j =0; (r = strpbrk(s,",*\n")) && j < 32; j++)
    {
//        int n = (int)(r-s);
        switch(j)
        {
            case 1:
                wbt->gps.mode1_gsa = (*s == 'M') ? 0 : 1;
                break;
            case 2:
                wbt->gps.mode2_gsa = *s - '0';
                break;
            case 3 ... 14: /* Warning: GCCism */
                wbt->gps.prn[j-3]= strtol(s,NULL,10);
                break;
            case 15:
                wbt->gps.pdop = strtod_nonlocale(s,NULL);
                break;
            case 16:
                wbt->gps.hdop = strtod_nonlocale(s,NULL);
                break;
            case 17:
                wbt->gps.vdop = strtod_nonlocale(s,NULL);
                break;
        }
        s = ++r;
    }
    if (j)
        wbt->gps.state++;
}

static void process_gsv(char *msg)
{
    int i=0,j,k;
    char *r,*s = msg;
    int total=0, this=-1;
    short done = 0;

    for(j =0; !done && (r = strpbrk(s,",*\n")); j++)
    {
        switch(j)
        {
            case 1:
                total = strtol(s,NULL,10);
                break;
            case 2:
                this = strtol(s,NULL,10);
                i = (this - 1)*4;
                break;
            case 3:
                 wbt->gps.nview = strtol(s,NULL,10);
                 break;
            case 4:
            case 8:
            case 12:
            case 16:
                wbt->gps.sinfo[i].prn =  strtol(s,NULL,10);
                wbt->gps.sinfo[i].in_use = 0;
                if (wbt->gps.sinfo[i].prn)
                {
                    for(k = 0; k < wbt->gps.nsat; k++)
                    {
                        if (wbt->gps.sinfo[i].prn == wbt->gps.prn[k])
                        {
                            wbt->gps.sinfo[i].in_use = 1;
                            break;
                        }
                    }
                }
                break;
            case 5:
            case 9:
            case 13:
            case 17:
                wbt->gps.sinfo[i].elev =  strtol(s,NULL,10);
                break;
            case 6:
            case 10:
            case 14:
            case 18:
                wbt->gps.sinfo[i].azimuth =  strtol(s,NULL,10);
                break;
            case 7:
            case 11:
            case 15:
            case 19:
                wbt->gps.sinfo[i].snr =  strtol(s,NULL,10);
                i++;
                done = (i == wbt->gps.nview);
                break;
        }
        s = ++r;
    }
    if (j && (this == total))
    {
        wbt->gps.state++;
#if 0
        printf("In view %d/%d\n",wbt->gps.nsat, wbt->gps.nview);
        for(k =0 ; k < 16; k++)
        {
            printf("%02d ", wbt->gps.prn[k]);
        }
        fputc('\n', stdout);
        for(k =0 ; k < 16; k++)
        {
            printf("%02d ", wbt->gps.sinfo[k].prn);
        }
        fputc('\n', stdout);
        for(k =0 ; k < 16; k++)
        {
            printf("%02d ", wbt->gps.sinfo[k].snr);
        }
        fputc('\n', stdout);
        fputc('\n', stdout);
#endif
    }
}

static void nmea_parse(gchar *s)
{
    if(strncmp("RMC", s+2, sizeof("RMC")-1) == 0)
    {
        process_rmc(s);
    }
    else if(strncmp("GGA", s+2, sizeof("GGA")-1) == 0)
    {
        process_gga(s);
    }
    else if(strncmp("GSA", s+2, sizeof("GSA")-1) == 0)
    {
        process_gsa(s);
    }
    else if(strncmp("GSV", s+2, sizeof("GSV")-1) == 0)
    {
        process_gsv(s);
    }
}

static int checks(gchar *s)
{
    guchar c,n = 0;
    guint chk;
    while((c = *s++) != '*')
    {
        n ^= c;
    }
    chk = strtol((char *)s,NULL,16);
    return (n == chk);
}

static void process_nmea(gchar *s)
{
    gint lc;
    GtkTextIter is,ie;
    gchar *p;
    static char mybuf[512];
    static int mybuflen;
    int slen;

    if((p = index(s,'\r')))
    {
        *p = '\n';
    }
    else
    {
        strcat(s,"\n");
    }

    if(mybuflen + (slen = strlen(s)) + 2 > sizeof(mybuf) )
    {
        lc = gtk_text_buffer_get_line_count (wbt->tb);
        if (lc > 1024)
        {
            gtk_text_buffer_get_start_iter (wbt->tb, &is);
            gtk_text_buffer_get_iter_at_line(wbt->tb, &ie, 256);
            gtk_text_buffer_delete (wbt->tb, &is, &ie);
        }
        gtk_text_buffer_get_end_iter (wbt->tb, &is);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(wbt->tv), &is, 0.0, TRUE, 0.0, 0.5);
        gtk_text_buffer_insert (wbt->tb, &is, mybuf, mybuflen);
        mybuflen = 0;
    }
    else
    {
        strcpy(mybuf+mybuflen, s);
        mybuflen += slen;
    }

    if(checks(s+1))
    {
        if(wbt->nmea_fp)
        {
            fputs(s,wbt->nmea_fp);
        }
        nmea_parse(s+1);
        if (wbt->gps.state == 3)
        {
            nmea_display();
            wbt->gps.state = -1;
        }
    }
}

static void process_al(char *s)
{
    static char *ini[] = {"7,1","7,2","8,1","8,2","8,3",
                          "5,1","5,2",
                          "1,1","1,2",
                          "6,1","6,2","6,3","6,4","6,5","6,6","6,7","6,9","6,10",
                          "6,11","6,12","6,13","6,14",NULL};
    char **p;
    GtkWidget *w;

    if(wbt->verbose) wbt_debug("al :%s\n", s);
    if (*s == '@' && *(s+1) == 'A' && *(s+2) == 'L')
    {
        if  (*(s+3) == ',')
        {
            s = s + 4;
            switch(*s)
            {
                case 'L':
                    wbt->dologin = FALSE;
                    if(wbt->connstat == 0)
                    {
                        wbt->connstat = 1;
                        for(p=ini;*p;p++)
                        {
                            wbt->serq = g_slist_append(wbt->serq,*p);
                        }
                    }
                    break;
                case '1':
                    if(*(s+1) == ',')
                    {
                        char *v,*z;
                        int act = strtol(s+2,&v,10);
                        if(*v++ == ',')
                        {
                            switch(act)
                            {
                                case 1:
                                case 2:
                                    wbt->cfg.wsvals[act] = strtol(v,&z,10);
                                    *z = 0;
                                    w=GTK_WIDGET (gtk_builder_get_object (
                                                      wbt->builder, wsets[act]));
                                    gtk_entry_set_text(GTK_ENTRY(w), v);
                                    break;
                            }
                        }
                    }
                    break;
                case '5':
                    if(*(s+1) == ',')
                    {
                        char *v,*z;
                        int act = strtol(s+2,&v,10);
                        if(*v++ == ',')
                        {
                            switch(act)
                            {
                                case 1:
                                    wbt->cfg.log_start = strtol(v,&z,10);
                                    *z = 0;
                                    w=GTK_WIDGET (gtk_builder_get_object (
                                                      wbt->builder,"log_start1"));
                                    gtk_entry_set_text(GTK_ENTRY(w), v);
                                    break;
                                case 2:
                                    wbt->cfg.log_end = strtol(v,&z,10);
                                    *z = 0;
                                    w=GTK_WIDGET (gtk_builder_get_object (
                                                      wbt->builder,"log_end1"));
                                    gtk_entry_set_text(GTK_ENTRY(w), v);
                                    break;
                            }
                        }
                    }
                    break;
                case '7':
                    if(*(s+1) == ',' && *(s+3) == ',')
                    {
                        switch(*(s+2))
                        {
                            case '1':
                                strcpy(wbt->cfg.device_name,s+4);
                                w=GTK_WIDGET (gtk_builder_get_object (
                                                  wbt->builder,"dev_name1"));
                                gtk_entry_set_text(GTK_ENTRY(w), s+4);
                                break;
                            case '2':
                                strcpy(wbt->cfg.device_info,s+4);
                                w=GTK_WIDGET (gtk_builder_get_object (
                                                  wbt->builder,"dev_info1"));
                                gtk_entry_set_text(GTK_ENTRY(w), s+4);
                                break;
                        }
                    }
                    break;
                case '8':
                    if(*(s+1) == ',' && *(s+3) == ',')
                    {
                        switch(*(s+2))
                        {
                            case '1':
                                strcpy(wbt->cfg.hw_version,s+4);
                                w=GTK_WIDGET (gtk_builder_get_object (
                                                  wbt->builder,"hw_version1"));
                                gtk_entry_set_text(GTK_ENTRY(w), s+4);
                                break;
                            case '2':
                                strcpy(wbt->cfg.sw_version,s+4);
                                w=GTK_WIDGET (gtk_builder_get_object (
                                                  wbt->builder,"sw_version1"));
                                gtk_entry_set_text(GTK_ENTRY(w), s+4);
                                break;
                            case '3':
                                strcpy(wbt->cfg.log_version,s+4);
                                w=GTK_WIDGET (gtk_builder_get_object (
                                                  wbt->builder,"log_version1"));
                                gtk_entry_set_text(GTK_ENTRY(w), s+4);
                                break;
                        }
                    }
                    break;
                case '6':
                    if(*(s+1) == ',')
                    {
                        char *v,*z;
                        int act = strtol(s+2,&v,10);
                        if(*v++ == ',')
                        {
                            switch(act)
                            {
                                case 1:
                                    wbt->cfg.wvvals[1] = strtol(v,NULL,10);
                                    assert(NULL != (w = GTK_WIDGET (
                                                        gtk_builder_get_object (
                                                            wbt->builder, "logcombo1"))));
                                    gtk_combo_box_set_active(GTK_COMBO_BOX(w),
                                                             wbt->cfg.wvvals[1]);
                                    set_edit_state(wbt->cfg.wvvals[1]);
                                    break;
                                case 2 ... 7:
                                case 9 ... 14:
                                    wbt->cfg.wvvals[act] = strtol(v,&z,10);
                                    *z = 0;
                                    assert(NULL != (w = GTK_WIDGET (
                                                        gtk_builder_get_object(
                                                            wbt->builder, wmodes[act]))));
                                    gtk_entry_set_text(GTK_ENTRY(w), v);
                                    break;
                            }
                        }
                    }
                    break;
            }
        }
//        else /* MAYBE */
        {
            if(wbt->dologin)
                perform_login();
        }
    }

}

void serial_process(char *s)
{
    char *p;
    if((p = index(s,'@')) && p != s)
        s = p;

    switch (*s)
    {
        case '$':
            process_nmea(s);
            if(wbt->dologin)
                perform_login();
            break;
        case '@':
            process_al(s);
            break;
    }
    kick_actions();
}
