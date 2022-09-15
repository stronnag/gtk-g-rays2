/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2012  Intel Corporation. All rights reserved.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/signalfd.h>

#include <glib.h>
#include <gdbus.h>

static GMainLoop *main_loop;
static DBusConnection *dbus_conn;

static void print_device(GDBusProxy *proxy, const char *description)
{
	DBusMessageIter iter;
	const char *address, *name;

	if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&iter, &address);

	if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
		dbus_message_iter_get_basic(&iter, &name);
	else
		name = "<unknown>";

	printf("%s%s%sDevice %s %s\n",
				description ? "[" : "",
				description ? : "",
				description ? "] " : "",
				address, name);
}

static void proxy_added(GDBusProxy *proxy, void *user_data)
{
    const char *interface;
    
    interface = g_dbus_proxy_get_interface(proxy);
    
    if (!strcmp(interface, "org.bluez.Device1"))
    {
        print_device(proxy,"DEV");
    }
}

static void proxy_removed(GDBusProxy *proxy, void *user_data)
{
    const char *interface;
    interface = g_dbus_proxy_get_interface(proxy);
    if (!strcmp(interface, "org.bluez.Device1"))
    {
        puts("Dev remove\n");
    } else if (!strcmp(interface, "org.bluez.Adapter1"))
    {
        puts("Dev remove\n");
    }
}

static void property_changed(GDBusProxy *proxy, const char *name,
					DBusMessageIter *iter, void *user_data)
{
    const char *interface;
    
    interface = g_dbus_proxy_get_interface(proxy);
    
    if (!strcmp(interface, "org.bluez.Device1"))
    {
      puts("DevProp change\n"):
        
    } else if (!strcmp(interface, "org.bluez.Adapter1"))
    {
      puts("CtlProp change\n"):            
    }
}

int main(int argc, char *argv[])
{
    GError *error = NULL;
    GDBusClient *client;

    main_loop = g_main_loop_new(NULL, FALSE);
    dbus_conn = g_dbus_setup_bus(DBUS_BUS_SYSTEM, NULL, NULL);
    
    client = g_dbus_client_new(dbus_conn, "org.bluez", "/org/bluez");
    
    g_dbus_client_set_connect_watch(client, connect_handler, NULL);
    g_dbus_client_set_disconnect_watch(client, disconnect_handler, NULL);
    g_dbus_client_set_signal_watch(client, message_handler, NULL);
    
    g_dbus_client_set_proxy_handlers(client, proxy_added, proxy_removed,
                                         property_changed, NULL);
    
    g_main_loop_run(main_loop);
    
    g_dbus_client_unref(client);
    dbus_connection_unref(dbus_conn);
    g_main_loop_unref(main_loop);
}
