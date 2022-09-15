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
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <assert.h>
#include <wait.h>
#include <unistd.h>
#include "wbt201.h"
#include <gudev/gudev.h>
#include <sys/signalfd.h>

#define VENDOR_ID "10c4"
#define MODEL_ID  "ea60"

#define handle_error(msg)                                       \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


static int get_ttyusb(char* devs)
{
    GUdevClient* uc;
    GUdevDevice* gu;
    static const char * const subsys[] = {"usb",NULL};
    int ndev = 0;
    uc = g_udev_client_new(subsys);
    if (uc)
    {
        char devname[32] = "/dev/ttyUSBx";
        int i;
        
        for(i=0; i < 10; i++)
        {
            devname[11]='0'+i;
            gu = g_udev_client_query_by_device_file(uc, devname);
            if(gu)
            {
                const char *val;
                val = g_udev_device_get_property(gu, "ID_VENDOR_ID");
                if( 0 == strcmp(val,VENDOR_ID))
                {
                    val = g_udev_device_get_property(gu, "ID_MODEL_ID");
                    if (0 == strcmp(val, MODEL_ID))
                    {
                        devs[ndev]=i;
                        ndev++;
                    }
                }
                g_object_unref(gu);
            }
        }
        g_object_unref(uc);
    }
    return ndev;
}

void clean_bluez(void)
{
    g_ray_disconnect (wbt->g);
    wbt->g = NULL;
}

void reset_quit_status(void)
{
    GtkWidget *q;
    assert(NULL != (q=GTK_WIDGET (gtk_builder_get_object
                                  (wbt->builder,
                                   "button3"))));
    gtk_button_set_label (GTK_BUTTON(q),"gtk-quit");
}


static void check_dev(char *dev)
{
    if(0 == strcasecmp(dev,"auto"))
    {
        char devs[16];
        memset(devs,0xff,sizeof(devs));
        if(get_ttyusb(devs) == 1)
        {
                /* set up fd and reset UI */
            wbt->curdev = g_malloc(32);
            sprintf(wbt->curdev,"/dev/ttyUSB%c", devs[0]+'0');
            done_serial();
            complete_serial_connect();
        }
    }

    if(wbt->curdev == NULL)
    {
        if( (strcasecmp(dev,"auto") == 0)  ||
            (strcasecmp(dev,"bluetooth") == 0))
        {
            GtkWidget *q;
            assert(NULL != (q=GTK_WIDGET (gtk_builder_get_object
                                          (wbt->builder,
                                          "button3"))));
            gtk_button_set_label (GTK_BUTTON(q), "gtk-cancel");
            if(wbt->g == NULL)
            {
                wbt->g = g_ray_new();
                if(wbt->g)
                    g_ray_connect (wbt->g);
            }
        }
        else
        {
            if(access(dev,R_OK|W_OK) == 0)
            {
                wbt->curdev = g_strdup(dev);
                done_serial();
                complete_serial_connect();
            }
        }
    }
}

void setup_serial(int fd,int baudrate)
{
    struct termios tio;   
    memset (&tio, 0, sizeof(tio));
    cfmakeraw(&tio);
    tio.c_cflag |= (CS8 | CLOCAL | CREAD);
    tio.c_iflag |= IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VTIME]    = 100;
    tio.c_cc[VMIN]     = 1;
    switch (baudrate)
    {
        case 0:      baudrate=B57600; break;
        case 4800:   baudrate=B4800; break;              
        case 9600:   baudrate=B9600; break;
        case 19200:  baudrate=B19200; break;
        case 38400:  baudrate=B38400; break;
        case 57600:  baudrate=B57600; break;
        case 115200: baudrate=B115200; break;
        case 230400: baudrate=B230400; break;
    }
    cfsetispeed(&tio,baudrate);
    cfsetospeed(&tio,baudrate);
    tcsetattr(fd,TCSANOW,&tio);
}

void done_serial(void)
{
    if(wbt->curdev)
    {
        wbt->serfd = open(wbt->curdev, O_RDWR|O_NOCTTY|O_NONBLOCK|O_NDELAY);
        if (wbt->serfd != -1)
        {
            wbt->sspeed = 57600;
            setup_serial(wbt->serfd, wbt->sspeed);
        }
        else
        {
            serial_tidy();
        }
    }
}

void init_serial (char *name, int baudrate)
{
    if(wbt->curdev)
    {
        g_free(wbt->curdev);
        wbt->curdev = NULL;
    }
    check_dev(name);
}

void serial_tidy(void)
{
    if(wbt->verbose) wbt_debug("close serial for %d\n", wbt->serfd);
    if ( wbt->serfd != -1)
    {
        tcflush(wbt->serfd,TCIOFLUSH);
        g_io_channel_shutdown(wbt->gio, FALSE, NULL);
        g_io_channel_unref(wbt->gio);
        close(wbt->serfd);
        wbt->serfd = -1;
    }
    if(wbt->verbose) wbt_debug("serial tidy: BT %p\n", wbt->g);
    if(wbt->g)
    {
        clean_bluez();
    }

    if(wbt->curdev)
    {
        g_free(wbt->curdev);
        wbt->curdev = NULL;
    }
}

void close_serial(void)
{
    GtkWidget *q,*s;
    set_menu_states(FALSE);
    assert(NULL != (q=GTK_WIDGET (gtk_builder_get_object (wbt->builder,"conn_button1"))));
    assert(NULL != (s = GTK_WIDGET (gtk_builder_get_object (
                                        wbt->builder,"conn_state1"))));
    gtk_button_set_label (GTK_BUTTON(q),"gtk-connect");
    assert(NULL != (q = GTK_WIDGET (gtk_builder_get_object (wbt->builder,"conn_state1"))));
    gtk_image_set_from_icon_name(GTK_IMAGE(s), "gtk-no", GTK_ICON_SIZE_MENU);
    gtk_main_iteration_do(FALSE);

    serial_tidy();
    wbt->connstat = 0;
}


void write_serial(char *msg, int len)
{
    int i;

    if(len == -1)
        len = strlen(msg);
    
    for( i = 0 ; i < 3 ; i++)
    {
        int n;

        do {
            n = write(wbt->serfd, msg, len);
        } while (n == -1 && (errno == EAGAIN || errno == EINTR));

        if(n == -1)
        {
            wbt_debug("write serial %d %s\n", wbt->serfd, strerror(errno));
        }
        else
        {
            if(wbt->verbose)
                wbt_debug ("Send IO %d %d %s", wbt->serfd, n, msg);
            break;
        }
    }
}

void start_serial_handler()
{
    wbt->gio = g_io_channel_unix_new(wbt->serfd);
    g_io_channel_set_encoding(wbt->gio,NULL,NULL);
    g_io_channel_set_buffered(wbt->gio,FALSE);
    wbt->tag = g_io_add_watch (wbt->gio, G_IO_IN|G_IO_HUP|G_IO_ERR|G_IO_NVAL,
                               read_data, NULL);
}

gboolean read_data(GIOChannel *source, GIOCondition condition, gpointer data)
{
    static gchar statbuf[1024];
    static gchar *eptr = statbuf;
    gchar buf[129];
    int len;
    int fd = g_io_channel_unix_get_fd(source);
    
    if((condition &  G_IO_IN) == G_IO_IN)
    {
        do {
            len = read(fd, buf, sizeof(buf)-1);
        } while (len == -1 && (errno == EAGAIN || errno == EINTR));

        if(len > 0)
        {
            gchar *sp,*ptr;
            sp = buf;
            buf[len] = 0;
            while((ptr = index(sp, '\n')))
            {
                *ptr = 0;
                eptr = stpcpy(eptr, sp);
                if((eptr - statbuf) > 0)
                {
                    if(*(eptr-1) == '\r')
                        *(eptr-1) = 0;
                }
                if(wbt->verbose) wbt_debug("read [%s]\n", statbuf);
                serial_process(statbuf);
                eptr = statbuf;
                sp = ptr+1;
            }
            if (*sp != '\0')
            {
                eptr = stpcpy(eptr, sp);
            }
            return TRUE;
        }
        else if(len == 0)
        {
            if(wbt->verbose) wbt_debug("read EOF (condition 0x%x %d)\n",
                                       condition, condition);
            close_serial();
            return FALSE;
        }
        else
        {
            if(wbt->verbose)
                wbt_debug("serial fd %d %s (condition 0x%x %d)\n",
                          fd, strerror(errno),condition, condition);
            close_serial();
            return FALSE;
        }
    }
    else
    {
        if(wbt->verbose)
            wbt_debug("other condition = 0x%x %d\n", condition,condition);
        return FALSE;
    }
}

#if 0
static gboolean sig_read_data(GIOChannel *source, GIOCondition condition, gpointer data)
{
    int sfd = g_io_channel_unix_get_fd(source);
    int s;
    struct signalfd_siginfo fdsi;

    if(wbt->verbose) wbt_debug("SIG HANDLER\n");
            
    s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
    if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGQUIT ||
        fdsi.ssi_signo == SIGTERM)
    {
        if(wbt->verbose) wbt_debug("Terminating\n");
        return FALSE;
    }
    else
    {
        if(wbt->verbose) wbt_debug("Read unexpected signal\n");
        return TRUE;
    }
}
#endif

void setup_signals(void)
{
#if 0
    sigset_t mask;
    int sfd;

    return;
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);

    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) == -1)
        handle_error("sigprocmask");

    sfd = signalfd(-1, &mask, 0);
    if (sfd == -1)
        handle_error("signalfd");

    GIOChannel *gio = g_io_channel_unix_new(sfd);
    g_io_add_watch (gio, G_IO_IN, sig_read_data, NULL);
#endif
}

