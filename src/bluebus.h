#include <glib.h>
#include <gtk/gtk.h>

typedef struct
{
    gchar *rfcom;
    short status;
} G_rays;

extern G_rays *  g_ray_new(void);
extern int g_ray_connect (G_rays *);
extern void g_ray_disconnect (G_rays *);

