#include <stdio.h>
#include <glib.h>

typedef void * G_ray;

extern G_ray* g_ray_new (void);
extern gint g_ray_connect (G_ray* self);
extern void g_ray_run (G_ray* self);
extern gchar* g_ray_get_device (G_ray* self);
extern void g_ray_disconnect (G_ray* self);
extern void g_ray_unref (gpointer instance);
