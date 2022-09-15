#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include "wbt201.h"
#include <gtk/gtk.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

void reset_quit()
{
    GtkWidget *q;
    assert(NULL != (q=GTK_WIDGET (gtk_builder_get_object
                                  (wbt->builder,
                                   "button3"))));
    gtk_button_set_label (GTK_BUTTON(q),"gtk-quit");
}

static gboolean create_bt_dev(void* unused)
{
    
    struct sockaddr_rc addr = { 0 };
    int s, status = -1;

    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (s < 0)
    {
        wbt_debug("Socket fails %d (%s)\n", s, strerror(errno));

    }
    else
    {
            // set the connection parameters (who to connect to)
        addr.rc_family = AF_BLUETOOTH;
        addr.rc_channel = (uint8_t) 1;
        if(wbt->verbose)
            wbt_debug("BT address %s\n", wbt->btaddr);                
        str2ba( wbt->btaddr, &addr.rc_bdaddr );
            // connect to server
        status = connect(s, (struct sockaddr *)&addr, sizeof(addr));
        if(status == 0)
        {
            wbt->serfd = s;
        }
        else
        {
            wbt->fd = -1;
            close(s);
        }
    }
    complete_serial_connect();
    reset_quit_status();
    return FALSE;
}

gboolean bt_discover(void *user_data)
{
   inquiry_info *ii = NULL;
    int max_rsp, num_rsp;
    int dev_id=-1, sock=-1, len, flags;
    int i;
    char addr[19] = { 0 };
    char name[248] = { 0 };

    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );
    if (dev_id < 0 || sock < 0) {
        wbt_debug("hci socket fails %d %d (%s)\n", dev_id, sock, strerror(errno));
        if(sock >= 0)
            close(sock);
    }
    else
    {
        len  = 8;
        max_rsp = 255;
        flags = 0; // IREQ_CACHE_FLUSH; // 0 => no flush, may return earlier data, faster
        ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
        
        num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
        if( num_rsp < 0 ) perror("hci_inquiry");

        for (i = 0; i < num_rsp; i++) {
            ba2str(&(ii+i)->bdaddr, addr);
            memset(name, 0, sizeof(name));
            if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), 
                                     name, 0) >= 0)
            {
                if (strcmp(name,"G-Rays2") == 0)
                {
                    wbt->btaddr = strdup(addr);
                    if(wbt->verbose)
                        wbt_debug("Found G-Rays2 => %s\n", wbt->btaddr);
                }
            }
        }
        free( ii );
        close( sock );
    }

    if (wbt->btaddr)
    {
        g_idle_add(create_bt_dev, NULL);
    }
    else
    {
        complete_serial_connect();
        reset_quit_status();
    }
    return FALSE;
}

void open_bluez_dev(G_rays *g)
{
    if(wbt->defbtaddr)
        wbt->btaddr = wbt->defbtaddr;
    
    if(wbt->btaddr == NULL)
        g_idle_add(bt_discover, NULL);
    else
        g_idle_add(create_bt_dev, NULL);

}

void g_ray_disconnect(G_rays *g)
{
    if(g->rfcom)
    {
        g_free (g->rfcom);
        g->rfcom = NULL;
    }
    g_free(g);
}


G_rays * g_ray_new(void)
{
    G_rays *g = g_malloc0(sizeof(G_rays));
    return g;
}

int g_ray_connect(G_rays *g)
{
    open_bluez_dev(g);
    return 0;
}


