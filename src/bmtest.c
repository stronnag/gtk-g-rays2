#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include "grays_marshal.h"
#include <gtk/gtk.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <gio/gunixfdlist.h>

#define DBUS_SERVICE_BZ   "org.bluez"
#define DBUS_PATH_MGR      "/"
#define DBUS_INTERFACE_MGR "org.bluez.Manager"


typedef struct
{
    DBusGProxy *device;
    DBusGProxy *adapter;
    gchar *path;
    GUnixFDList *fdlist;
    guint ndisc;
    int i_disco;
} G_rays;

typedef struct
{
    DBusGConnection *bus;    
    G_rays *g;
    int fd;
} WBT;

static WBT _w, *wbt = &_w;

extern G_rays *g_ray_new(void);
extern int g_ray_connect (WBT *);
extern void g_ray_disconnect (G_rays *);

#define handle_error(msg)                                       \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

static  GMainLoop* main_loop;
static GError *error;
static int logfd;

static void write_error (char *message,
			 GError **error)
{
    if (error == NULL || *error == NULL) {
	return;
    }
    g_printerr ("%s: %s\n", message, (*error)->message);
    g_clear_error (error);
}

static void bt_quit(void *user_data)
{
    if(wbt->fd != -1)
        close(wbt->fd);
    if (wbt->g)
    {
        g_ray_disconnect (wbt->g);
        wbt->g=NULL;
    }
    g_main_loop_quit(main_loop);
}

void setup_serial(int fd,int baudrate)
{
    struct termios tio;   
    memset (&tio, 0, sizeof(tio));
    tio.c_cflag = CS8 | CLOCAL | CREAD | O_NDELAY | O_NONBLOCK;
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VTIME]    = 0;
    tio.c_cc[VMIN]     = 1;
    switch (baudrate)
    {
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

int init_serial (char *name, int baudrate)
{
    int fd =-1;
    
    if(name)
    {
        fd = open(name, O_RDWR|O_NOCTTY|O_NONBLOCK);
        if (fd != -1)
        {
            setup_serial(fd,baudrate);
        }
    }
    return fd;
}

gboolean sig_read_data(GIOChannel *source, GIOCondition condition, gpointer data)
{
    int len;
    int sfd = g_io_channel_unix_get_fd(source);
    int s;
    struct signalfd_siginfo fdsi;

    fprintf(stderr,"SIG HANDLER\n");
    fflush(stderr);
        
    s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
    if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGQUIT ||
        fdsi.ssi_signo == SIGTERM)
    {
        fprintf(stderr,"Terminating\n");
        bt_quit(NULL);
        return FALSE;
    }
    else
    {
        fprintf(stderr, "Read unexpected signal\n");
        return TRUE;
    }
}

static void serial_process(char *s)
{
    static int val;
    static char *ini[] = {
        "7,1","7,2","8,1","8,2","8,3",
        "5,1","5,2",
        "1,1","1,2",
        "6,1","6,2","6,3","6,4","6,5","6,6","6,7","6,9","6,10",
        "6,11","6,12","6,13","6,14",NULL};
    
    fprintf(stderr,"Complete IO [%s]\n", s);                    

    if (*s == '@' && *(s+1) == 'A' && *(s+2) == 'L' && *(s+3) == ',')
    {
        fprintf(stderr,"value = %d\n", val);
        if(ini[val])
        {
            char obuf[128];
            strcpy(obuf, "@AL,");
            strcat(obuf,ini[val]);
            fprintf(stderr,"Send IO [%s]\n", obuf);
            strcat(obuf,"\r\n\n");
            write(wbt->fd, obuf, strlen(obuf));
            val++;
        }
        else
            fprintf(stderr,"EOD\n");
    }
    else
    {
        val = 0;
    }
}

gboolean read_data(GIOChannel *source, GIOCondition condition, gpointer user_data)
{
    static gchar statbuf[1024];
    static gchar *eptr = statbuf;
    
    gchar buf[129];
    int len;
    int fd = g_io_channel_unix_get_fd(source);
    
    do {
        len = read(fd, buf, sizeof(buf)-1);
    } while (len == -1 && (errno == EAGAIN || errno == EINTR));

    if(len > 0)
    {
        write(logfd, buf,len);
        write(logfd,"|",1);
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
    else 
    {
         return FALSE;
    }
}

static start_io(void *user_data)
{
    write(wbt->fd, "@AL\n", 4);
    return FALSE;
}

static int open_serial(char *name, int rate)
{
    int fd = init_serial(name, rate);
    if (fd != -1)
    {
        fprintf(stderr,"FD %d %s\n", fd, name);        
        fprintf(stderr,"Start reading\n");
        GIOChannel *gio = g_io_channel_unix_new(fd);
        g_io_add_watch (gio, G_IO_IN|G_IO_HUP|G_IO_ERR|G_IO_NVAL,
                        read_data, NULL);
        g_timeout_add_seconds(5,start_io, NULL);
    }
    else
    {
        fprintf(stderr,"open fail %d %s\n", fd, strerror(errno));        
    }
    
    return fd;
}

#define MAXTRY 100
gboolean try_open_serial(void *unused)
{
    static int ntry;
    gboolean ret;
    
//    wbt->fd = open_serial(wbt->g->rfcom,57600);
    if (wbt->fd == -1)
    {
        ntry += 1;
        if (ntry < MAXTRY)
        {
//            fprintf(stderr,"giving up on %s\n", wbt->g->rfcom);
            bt_quit(NULL);
            ret = FALSE;
        }
        else
            ret = TRUE;
    }
    else
    {
        ntry - 0;
        ret = FALSE;
    }
    return ret;
}

static void prop_changed (DBusGProxy *object,
                          const char *prop,
                          GValue *value,
                          void *user_data)
{
    WBT *w = (WBT *)user_data; 
    if(0 == strcmp(prop,"Discovering"))
    {
        gboolean disco = g_value_get_boolean (value);
        fprintf(stderr,"Disco value %d / %d \n", disco, w->g->ndisc);
        if(disco)
        {
            w->g->ndisc++;
            if(w->g->ndisc == 3)
            {
                w->g->i_disco = 0;
                dbus_g_proxy_call (w->g->adapter, "StopDiscovery", NULL,
                                   G_TYPE_INVALID, G_TYPE_INVALID);
                fprintf(stderr,"Stop discovery\n");
                    // reset buttons
            }
        }
        else
        {
            w->g->i_disco = 0;
            fprintf(stderr,"End of disco\n");
//            if (w->g->rfcom == NULL)
            {
                bt_quit(w);
            }
        }
    }
}

static void device_found (DBusGProxy *object,
                          const char *address, GHashTable *props,
                          void *user_data)
{
   GValue *value;
   const char *name;
   WBT *w = (WBT *)user_data;
   
   value = g_hash_table_lookup (props, "Name");
   name = value ? g_value_get_string (value) : NULL;
   if(name && strcmp(name, "G-Rays2") == 0)
   {
       fprintf(stderr,"Found %s\n", name);
       dbus_g_proxy_call (w->g->adapter, "StopDiscovery", NULL,
                      G_TYPE_INVALID, G_TYPE_INVALID);

       char buf[128];
       char *s,*d;
       d = stpcpy(buf, w->g->path);
       d = stpcpy(d, "/dev_");
       for(s=(char *)address; *s; s++, d++)
       {
	   *d = *s;
	   if(*d == ':')
		   *d = '_';
       }
       *d = 0;

       w->g->device = dbus_g_proxy_new_for_name (w->bus, "org.bluez",
					buf, "org.bluez.Serial");
       fprintf(stderr,"connecting %s %p\n", buf, w->g->device);
       if (!dbus_g_proxy_call (w->g->device, "ConnectFD", &error,
			       G_TYPE_STRING, "spp",
			       G_TYPE_INVALID,
			       G_TYPE_UNIX_FD_LIST, &(w->g->fdlist),
			       G_TYPE_INVALID))
       {
	   write_error ("BT Connect: ", &error);
           bt_quit(NULL);
       }
       else
       {
           fprintf(stderr,"Got %p\n", w->g->fdlist);
           {
               g_idle_add(try_open_serial, NULL);
               exit(0);
           }
       }
   }
   else if (name)
   {
       fprintf(stderr, "Name = %s\n", name);
   }
   else
   {
       fputs("No name", stderr);
   }
   
}

void g_ray_disconnect(G_rays *g)
{
    if(g->device)
    {
        g_object_unref (g->device);
        g->device  = NULL;
    }
    
    
    if(g->adapter)
    {
#if 0
        if(g->i_disco)
        {
            dbus_g_proxy_call (g->adapter, "StopDiscovery", NULL,
                               G_TYPE_INVALID, G_TYPE_INVALID);
        }
#endif
        g_object_unref (g->adapter);
        g->adapter = NULL;
    }

    if(g->path)
    {
        g_free (g->path);
        g->path = NULL;
    }
    g_free(g);
}


G_rays * g_ray_new(void)
{
    G_rays *g = g_malloc0(sizeof(G_rays));
    return g;
}

int g_ray_connect(WBT *w)
{
    int res = -1;
    DBusGProxy *mgr;

    if(w->bus == NULL)
    {
        w->bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
    }
    
    if (w->bus)
    {
	mgr = dbus_g_proxy_new_for_name (w->bus, "org.bluez", 
					 "/", "org.bluez.Manager");
	if(mgr)
        {
            if (!dbus_g_proxy_call (mgr, "DefaultAdapter",
                                    &error,
                                    G_TYPE_INVALID,
                                    DBUS_TYPE_G_OBJECT_PATH,
                                    &w->g->path, G_TYPE_INVALID))
            {
                write_error ("Default Adapter: ", &error);
            }
        }
        g_object_unref (mgr);
        mgr = NULL;
        if(w->g->path)
        {
            w->g->adapter = dbus_g_proxy_new_for_name (w->bus, "org.bluez",
                                                       w->g->path,
                                                       "org.bluez.Adapter");
            if(w->g->adapter)
            {
                dbus_g_object_register_marshaller(grays_marshal_VOID__STRING_BOXED,
                                                  G_TYPE_NONE, G_TYPE_STRING,
                                                  G_TYPE_VALUE, G_TYPE_INVALID);
        
                dbus_g_proxy_add_signal (w->g->adapter, "DeviceFound",
                                         G_TYPE_STRING,
                                         dbus_g_type_get_map
                                         ("GHashTable",
                                          G_TYPE_STRING, G_TYPE_VALUE),
                                         G_TYPE_INVALID);

                dbus_g_proxy_connect_signal (w->g->adapter, "DeviceFound",
                                             G_CALLBACK (device_found),
                                             w, NULL);
                
                dbus_g_proxy_add_signal (w->g->adapter, "PropertyChanged",
                                         G_TYPE_STRING, G_TYPE_VALUE,
                                         G_TYPE_INVALID);
                dbus_g_proxy_connect_signal (w->g->adapter, "PropertyChanged",
                                             G_CALLBACK (prop_changed),
                                             w, NULL);

                w->g->i_disco = 1;
                dbus_g_proxy_call (w->g->adapter, "StartDiscovery", &error,
                                   G_TYPE_INVALID, G_TYPE_INVALID);
                if (error != NULL)
                {
                    write_error("Discover: ", &error);
                }
                else
                {
                    res = 0;
                }
            }
        }
    }
    return res;
}

int init_bt( void *user_data)
{
    WBT *w = (WBT *)user_data;
    if(w->g && g_ray_connect (w) == 0)
    {
        fprintf(stderr, "INIT\n");
    }
    else
    {
        fprintf(stderr,"Failed to connect to BT\n");
        bt_quit(w);
    }
    return FALSE;
}

int main(int argc, char **argv)
{
    main_loop = g_main_loop_new (NULL, FALSE);
    g_type_init();
    
    logfd = open("/tmp/glog.txt",O_CREAT|O_WRONLY,0666);

    sigset_t mask;
    int sfd;

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
    g_io_add_watch (gio, G_IO_IN|G_IO_HUP|G_IO_ERR|G_IO_NVAL,
                    sig_read_data, NULL);

    wbt->fd = -1;
    
    if(argc == 2)
    {
        wbt->fd = open_serial(argv[1],57600);
    }
    else
    {
        wbt->g = g_ray_new();
        g_idle_add (init_bt,wbt);
    }
    g_main_loop_run(main_loop);
    if (wbt->bus)
    {
        dbus_g_connection_unref(wbt->bus);
    }
    return 0;
}

