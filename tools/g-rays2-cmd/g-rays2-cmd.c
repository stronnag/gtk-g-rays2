#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <ctype.h>

#define VERSION "0.02"

typedef struct
{
    char *name;
    char *cmd;
} wbt201_cmd_t;

static wbt201_cmd_t cmds[]={
    {.name="auto_sleep", .cmd="1,1"},
    {.name="over_speed", .cmd="1,2"},
    {.name="log_type", .cmd="6,1"},
    {.name="low_speed_limit", .cmd="6,2"},
    {.name="high_speed_limit", .cmd="6,3"},
    {.name="heading_mode_int", .cmd="6,4"},
    {.name="low_speed", .cmd= "6,5"},
    {.name="middle_speed", .cmd="6,6"},
    {.name="high_speed", .cmd="6,7"},
    {.name="speed_mode_time", .cmd="6,9"},
    {.name="low_middle_int", .cmd= "6,10"},
    {.name="middle_high_int", .cmd="6,11"},
    {.name="high_high_int", .cmd= "6,12"},
    {.name="time_mode_int", .cmd= "6,13"},
    {.name="dist_mode_int", .cmd= "6,14"},
    {.name=NULL, .cmd=NULL}
};

static void list_cmds(void)
{
    wbt201_cmd_t *c;
    for(c = cmds; c->name; c++)
    {
        puts(c->name);
    }
}

static struct termios xtio;   
static void setup_serial(int fd,int baudrate)
{
    struct termios tio;   

    tcgetattr(fd, &xtio);
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

static int alarmed;

void alarm_signal(int sig)
{
    alarmed = 1;
}

static int init_serial (char *name, int baudrate)
{
    int fd = open(name, O_RDWR|O_NOCTTY);
    if (fd != -1)
    {
        setup_serial(fd,baudrate);
    }
    return fd;
}

int fd_readline(int fd, char **str)
{
    char lbuf[512];
    int j = 0;
    int done = 0;
    char c;
    int res = -1;
    *str = NULL;

    alarmed = 0;
    alarm(5);
    while(!done)
    {
        int n = read(fd, &c, 1);
        if(n == 1)
        {
            if(c == '\n')
            {
                lbuf[j] = 0;
                done = 1;
                res = j;
                *str = strdup(lbuf);
            }
            else
            {
                lbuf[j++] = c;
            }
            if(j == 512)
            {
                done = 1;
                res = -1;
            }
        }
        else if (n == 0)
        {
            done = 1;
            res = 0;
        }
        else if (alarmed)
        {
            done = 1;
            res = -1;
        }
    }
    alarm(0);
    return res;
}


char *send_cmd(int fd, char *cmd, char *resp)
{
    int done = 0;
    char *str = NULL;
    char cmdbuf[64];
    strcpy(cmdbuf, cmd);
    strcat(cmdbuf, "\r\n");
        
    while(!done)
    {
        int res;
        res = write(fd, cmdbuf, strlen(cmdbuf));
        if(res > 0)
        {
            int n = fd_readline(fd, &str);
            if(n > 0)
            {
                if(strstr(str, resp))
                {
                    done = 1;
                }
                else
                {
                    free(str);
                }
            }
            else
            {
                done = 1;
                if(str)
                    free(str);
                str = NULL;
            }
        }
    }
    return str;
}

static int interrogate(int sfd)
{
    int res = 0;
    wbt201_cmd_t *c;
    for(c = cmds; c->name; c++)
    {
        char *line;
        char cmdstr[64];
        strcpy(cmdstr, "@AL,");
        strcat(cmdstr, c->cmd);
        line = send_cmd(sfd, cmdstr, c->cmd);
        if(line)
        {
            int n = 5 + strlen(c->cmd);
            if(n < strlen(line))
            {
                printf("%s: %s\n", c->name, line+n);
            }
            free(line);
        }
        else
        {
            res = -1;
            break;
        }
    }
    return res;
}

static int send_settings (int fd, char **set)
{
    int res = 0;
    char **s;
    for(s = set;(*s && (res == 0));s++)
    {
        char *value=NULL;
        int n;
        n = strcspn(*s," =:");
        value = (*s+n);
        *value = 0;
        while (*++value && (*value < '0' || *value > '9'))
            ; /* Skip to value */
        
        if(**s && *value)
        {
            wbt201_cmd_t *c;
            char *line;
            
            for(c = cmds; c->name; c++)
            {
                if(strcmp(c->name, *s) == 0)
                {
                    char buf[32];
                    sprintf(buf,"@AL,%s,%s", c->cmd, value);
                    line = send_cmd(fd, buf, buf);
                    if(line)
                    {
                        free(line);
                    }
                    else
                    {
                        res = -1;
                    }
                    break;
                }
            }
        }
    }
    return res;
}

int main (int argc, char *argv[])
{
    GError *error = NULL;
    GOptionContext *context;
    gboolean verbose=FALSE;
    gboolean showvers=FALSE;    
    gboolean show=FALSE;
    gboolean showcmds=FALSE;
    char **set=NULL;
    char *device = NULL,*xfile=NULL;
    int sfd = -1;
    char *line;
    struct sigaction sac;
    int res =-1;
    
    GOptionEntry entries[] = {
        { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "be verbose", NULL },
        { "version", 'V', 0, G_OPTION_ARG_NONE, &showvers, "version info", NULL },	
        { "list", 'l', 0, G_OPTION_ARG_NONE, &show, "show current", NULL },
        { "from-file", 'f', 0, G_OPTION_ARG_STRING, &xfile, "get settings frrom file", NULL },	
        { "names", 'n', 0, G_OPTION_ARG_NONE, &showcmds, "show settings names", NULL },        
        { "set", 's', 0, G_OPTION_ARG_STRING_ARRAY, &set, "set data (name=val), repeat as necessary", NULL },  
        { NULL }
    };

    context = g_option_context_new ("- configure wbt-201 GPS");
    g_option_context_set_ignore_unknown_options(context,FALSE);
    g_option_context_add_main_entries (context, entries,NULL);
    if(g_option_context_parse (context, &argc, &argv,&error) == FALSE)
    {
        if (error != NULL)
        {
            fprintf (stderr, "%s\n", error->message);
            g_error_free (error);
            exit(1);
        } 
    }

    if(showcmds || showvers)
    {
	if(showcmds)
	{
	    fputs("# ", stdout);
	}
	puts("g-rays2-cmd v" VERSION " (c) 2007 Jonathan Hudson");
	if(showcmds)
	{
	    fputc('\n',stdout);
	    list_cmds();
	}
        exit(0);
    }

    if(argc == 2)
    {
        if(argv[1][0] != '-')
        {
            device = g_strdup(argv[1]);
        }
    }
    else
    {
        fputs("Device is required\n", stderr);
        exit(1);
    }
    
    if(device)
    {
        sfd = init_serial(device, 57600);
    }
    else
    {
        sfd = 0;
        setup_serial(sfd,57600);
    }
    
    if(sfd == -1)
    {
        fputs("Failed to open device\n", stderr);
        exit(1);
    }

    sigemptyset(&(sac.sa_mask));
    sac.sa_flags=0;
    sac.sa_handler=alarm_signal;
    sigaction(SIGALRM, &sac, NULL);  
    
    line = send_cmd(sfd, "@AL", "@AL");

    if(line)
    {
        g_free(line);

	if(xfile)
	{
	    FILE *fp;
	    char **xset = NULL;
	    char **xp;
	    int nl = 0;
	    if((fp = fopen(xfile, "r")))
	    {
		char lbuf[256];
		while(fgets(lbuf,sizeof(lbuf),fp))
		{
		    if(*lbuf != '#' && !isspace(*lbuf))
		    {
			nl++;
		    }
		}
		rewind(fp);
		if(nl)
		{
		    xset = xp = calloc(nl+1, sizeof(char*));
		    while(fgets(lbuf,sizeof(lbuf),fp))
		    {
			if(*lbuf != '#' && !isspace(*lbuf))
			{
			    char *p;
			    if((p = index(lbuf,'\n')))
			    {
				*p = 0;
				*xp++ = strdup(lbuf);
			    }
			}
		    }
                    *xp = NULL;

		    send_settings(sfd, xset);		
		    for(xp = xset;*xp;xp++)
		    {
			free(*xp);
		    }
		    free(xset);
		}
		fclose(fp);
	    }
	}
	
        if(set)
        {
            send_settings(sfd, set);
        }

        if(show)
        {
            interrogate(sfd);
        }
    }
    else
    {
        fputs("Failed to initialise device\n\r", stderr);
    }

    if(res == 0)
    {
        line = send_cmd(sfd, "@AL,2,1", "@AL,2,1");
    }

    if(sfd != -1)
    {
        
        tcsetattr(sfd,TCSANOW,&xtio);
        close(sfd);
    }
    return 0;
}
