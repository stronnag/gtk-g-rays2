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
static void  spawn_hcitool(void);
static gboolean try_open_serial(void *);

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


static gboolean create_bt_dev(void* unused)
{
    char cmd[256];
    wbt->g->rfcom = strdup("/dev/rfcomm42");
    sprintf(cmd,"sudo rfcomm bind %s %s", wbt->g->rfcom, wbt->btaddr);
    int ret = system(cmd);
    if (ret == 0)
    {
        g_idle_add(try_open_serial, NULL);
    }
    return FALSE;
}

static void finalise_hcitool(GPid pid, gint status, gpointer data)

{
    char buf[1024];
    spawndata_t *s=(spawndata_t *)data;
    int n;
    char lbuf[64] = {0};

    FILE *fp = fdopen(s->sfd, "r");
        
    while(fgets(buf, sizeof(buf), fp) != NULL)
    {
        if(strstr(buf, "G-Rays2"))
        {
            sscanf(buf,"%s",lbuf);
        }
    }
    fclose(fp);
    g_spawn_close_pid(pid);
    if (*lbuf)
    {
        wbt->btaddr = strdup(lbuf);
        g_idle_add(create_bt_dev, NULL);
    }
    else
    {
        fprintf(stdout,"Retrying\n");
        spawn_hcitool();
    }
}

void spawn_hcitool(void)
{
    
    static char * argv[] = {"/usr/bin/hcitool","scan",NULL};
    static spawndata_t sd;
    GPid pid;
        
    g_spawn_async_with_pipes(NULL,
                             argv,
                             NULL,
                             G_SPAWN_DO_NOT_REAP_CHILD|G_SPAWN_STDERR_TO_DEV_NULL,
                             NULL,
                             &sd,
                             &pid,
                             NULL,
                             &sd.sfd,
                             NULL,
                             NULL);
    g_child_watch_add(pid, finalise_hcitool, &sd);
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

void setup_serial(int fd,int baudrate)
{
    struct termios tio;   
    memset (&tio, 0, sizeof(tio));
    tio.c_cflag = CS8 | CLOCAL | CREAD | O_NDELAY | O_NONBLOCK;
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VTIME]    = 10;
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
        g_timeout_add_seconds(10,start_io, NULL);
    }
    else
    {
        fprintf(stderr,"open fail %d %s\n", fd, strerror(errno));        
    }
    return fd;
}

#define MAXTRY 10
static gboolean try_open_serial(void *unused)
{
    static int ntry = 0;
    gboolean ret;
    
    wbt->fd = open_serial(wbt->g->rfcom,57600);
    if (wbt->fd == -1)
    {
        ntry += 1;
        if (ntry > MAXTRY)
        {
            fprintf(stderr,"giving up on %s\n", wbt->g->rfcom);
            bt_quit(NULL);
            ret = FALSE;
        }
        else
        {
            fputs("Retrying ...\n", stderr);
            ret = TRUE;
        }
    }
    else
    {
        fputs("opened ok ...\n", stderr);        
        ntry = 0;
        ret = FALSE;
    }
    return ret;
}

void open_bluez_dev(WBT *w)
{
    if(w->btaddr == NULL)
        spawn_hcitool();
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
    
    if(argc == 2 && argv[1][0] == '/')
    {
        wbt->fd = open_serial(argv[1],57600);
    }
    else
    {
        if(argc == 2)
            wbt->btaddr = strdup(argv[1]);
        wbt->g = g_ray_new();
        g_idle_add (init_bt,wbt);
    }
    g_main_loop_run(main_loop);
    return 0;
}

