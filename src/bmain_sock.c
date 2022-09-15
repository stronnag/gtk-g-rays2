#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

typedef struct
{
    gchar *rfcom;
} G_rays;

typedef struct
{
    G_rays *g;
    int fd;
    char *btaddr;
} WBT;

typedef struct 
{
    gint sfd;
} spawndata_t;

static WBT _w, *wbt = &_w;

extern G_rays *g_ray_new(void);
extern int g_ray_connect (WBT *);
extern void g_ray_disconnect (G_rays *);
static gboolean create_bt_dev(void*);
static void bt_quit(void *);
static start_io(void *);
extern gboolean read_data(GIOChannel *source, GIOCondition condition, gpointer user_data);


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
    g_printerr ("%s: [%d] %s\n", message, (*error)->code, (*error)->message);
    g_clear_error (error);
}

gboolean bt_discover(void *user_data)
{
   inquiry_info *ii = NULL;
    int max_rsp, num_rsp;
    int dev_id, sock, len, flags;
    int i;
    char addr[19] = { 0 };
    char name[248] = { 0 };

    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );
    if (dev_id < 0 || sock < 0) {
        perror("opening socket");
        bt_quit(NULL);
        return FALSE;
    }
    len  = 8;
    max_rsp = 255;
    flags = IREQ_CACHE_FLUSH;
    ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
    
    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
    if( num_rsp < 0 ) perror("hci_inquiry");

    for (i = 0; i < num_rsp; i++) {
        ba2str(&(ii+i)->bdaddr, addr);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), 
                                 name, 0) >= 0)
        {
            puts(name);
            if (strcmp(name,"G-Rays2") == 0)
            {
                wbt->btaddr = strdup(addr);
                printf("Got ya %s\n", wbt->btaddr);
            }
        }
        else
        {
            puts("No name");
        }
    }
    free( ii );
    close( sock );
    if (wbt->btaddr)
        g_idle_add(create_bt_dev, NULL);

    return FALSE;
}

static void bt_quit(void *user_data)
{
    fputs("Quit\n", stderr);
    if(wbt->fd != -1)
        close(wbt->fd);
    if (wbt->g)
    {
        g_ray_disconnect (wbt->g);
        wbt->g=NULL;
    }
    g_main_loop_quit(main_loop);
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

static gboolean create_bt_dev(void* unused)
{
    struct sockaddr_rc addr = { 0 };
    int s, status = -1;

    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (s < 0)
    {
        printf("Socket fails %d (%s)\n", s, strerror(errno));
    }
    else
    {
            // set the connection parameters (who to connect to)
        addr.rc_family = AF_BLUETOOTH;
        addr.rc_channel = (uint8_t) 1;
        printf("using %s\n", wbt->btaddr);                
        str2ba( wbt->btaddr, &addr.rc_bdaddr );
            // connect to server
        status = connect(s, (struct sockaddr *)&addr, sizeof(addr));
        if(status == 0)
        {
            GIOChannel *gio = g_io_channel_unix_new(s);
            wbt->fd = s;
            g_io_add_watch (gio, G_IO_IN|G_IO_HUP|G_IO_ERR|G_IO_NVAL,
                            read_data, NULL);
            g_timeout_add_seconds(10,start_io, NULL);
        }
        else
        {
            printf("connect fails %d (%s)\n", status, strerror(errno));
            wbt->fd = -1;
            close(s);
        }
    }
    return (status == 0) ? FALSE : TRUE;
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


void open_bluez_dev(WBT *w)
{
    if(w->btaddr == NULL)
        g_idle_add(bt_discover, NULL);
    else
        g_idle_add(create_bt_dev, NULL);

}

void g_ray_disconnect(G_rays *g)
{
    if(g->rfcom)
    {
        char cmd[256];
        sprintf(cmd, "sudo rfcomm release %s", g->rfcom);
        system (cmd);
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

int g_ray_connect(WBT *w)
{
    open_bluez_dev(w);
    return 0;
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
        wbt->btaddr = strdup(argv[1]);
    wbt->g = g_ray_new();
    g_idle_add (init_bt,wbt);
    g_main_loop_run(main_loop);
    return 0;
}

