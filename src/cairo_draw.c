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
#include <libintl.h>
#include <time.h>
#include <math.h>
#include "wbt201.h"

#ifdef HAVE_CAIRO
void sat_set_colour(cairo_t *cr, sat_info_t * s)
{
    if(s->in_use == 0)
    {
        cairo_set_source_rgb(cr,1,1,1);        
    }
    else
    {
        if(s->snr == 0)
        {
            cairo_set_source_rgb(cr,1,1,1);        
        }
        else if (s->snr < 25)
        {
            cairo_set_source_rgb(cr,0.2,0.2,0.8);
        }
        else
        {
            cairo_set_source_rgb(cr,0,0,1);
        }
    }
}

void graph_draw(GtkWidget *wgt, cairo_t *cr)
{
    double x, y, w, h;
    double scf, radius;
    double x1,x2,y1,y2;
    double rad,rng;
    char buf[16];
    int i;

    w = gtk_widget_get_allocated_width(wgt);
    h = gtk_widget_get_allocated_height(wgt);
    x = w/ 2;
    y = h / 4;
    scf= MIN(x,y);
    radius = scf - 10;
    scf *= 2;

    cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                       CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, scf/20);

    cairo_set_line_width (cr, 1.0);
    cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
    cairo_stroke(cr);
    cairo_set_line_width (cr, 0.5);
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_arc(cr, x, y, radius*2.0/3.0, 0, 2 * M_PI);
    cairo_stroke(cr);
    cairo_arc(cr, x, y, radius/3.0, 0, 2 * M_PI);
    cairo_stroke(cr);
    for(i= 0; i < 8; i++)
    {
        rad = i*M_PI/8.0;
        x1 = x + radius*sin(rad);
        y1 = y + radius*cos(rad);
        rad += M_PI;
        x2 = x + radius*sin(rad);
        y2 = y + radius*cos(rad);
        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);        
    }

    double sw = scf *0.9 / wbt->gps.nview;
    if (sw > 0.1*w) sw =  0.1*scf;
    double bht = h * 0.9;
    
    for(i =0; i < wbt->gps.nview; i++)
    {
        rad = wbt->gps.sinfo[i].azimuth * (M_PI / 180.0);
        rng = (90.0- wbt->gps.sinfo[i].elev)*(radius/90.0);
        x1 = x + rng*sin(rad);
        y1 = y - rng*cos(rad);
        sat_set_colour(cr, &wbt->gps.sinfo[i]);
        cairo_arc(cr, x1, y1, radius/8, 0, 2*M_PI);
        cairo_fill_preserve (cr);
        cairo_set_line_width(cr, w/100);
        cairo_set_source_rgb (cr, 0, 0, 0);
        cairo_stroke (cr);
        cairo_move_to(cr, x1-0.03*scf, y1+0.02*scf);
        if (wbt->gps.sinfo[i].snr && wbt->gps.sinfo[i].in_use)
            cairo_set_source_rgb(cr, 1, 1, 1);
        sprintf(buf,"%02d", wbt->gps.sinfo[i].prn);
        cairo_show_text(cr, buf);
        cairo_stroke(cr);

        double ht = (wbt->gps.sinfo[i].snr/100.0)*h/2;
        double xp = scf*0.06+i*sw;
        double bw = sw - scf*0.02;
        
        sat_set_colour(cr, &wbt->gps.sinfo[i]);
        cairo_rectangle(cr, xp, bht - ht , bw, ht);
        cairo_fill_preserve (cr);
        cairo_set_line_width(cr, scf/100);
        cairo_set_source_rgb (cr, 0, 0, 0);
        cairo_stroke(cr);

        cairo_move_to(cr, xp-scf*0.01, bht + scf*0.1);
        cairo_show_text(cr, buf);
        cairo_stroke(cr);

        if(wbt->gps.sinfo[i].snr)
        {
            cairo_move_to(cr, xp-scf*0.01, bht-ht-0.02*scf);
            sprintf(buf,"%02d", wbt->gps.sinfo[i].snr);
            cairo_show_text(cr, buf);
            cairo_stroke(cr);
        }
    }
}
#endif
