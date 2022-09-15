
BUILT_SOURCES = grays_marshal.c grays_marshal.h

bin_PROGRAMS = gtk-g-rays2
gtk_g_rays2_DEPENDENCY = gtk-g-rays2.ui

gtk_g_rays2dir=$(prefix)/bin

gtk_g_rays2_SOURCES = babel_spawn.c messages.c serial.c wbt201.c cairo_draw.c misc.c uihandler.c bluecon.c $(BUILT_SOURCES) wbt201.h bluebus.h

AM_CPPFLAGS = @WBT201_CFLAGS@ -DLOCALEDIR=\""$(datadir)/locale"\" -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"  -DHELPDIR=\""$(datadir)/help/C/gtk-g-rays2"\" $(CAIRO_CFLAGS)
gtk_g_rays2_LDADD = @WBT201_LIBS@ $(CAIRO_LIBS) 

ui_DATA = gtk-g-rays2.ui

uidir = $(pkgdatadir)

grays_marshal.h: grays_marshal.list
	glib-genmarshal $< --prefix=grays_marshal --header > $@

grays_marshal.c: grays_marshal.list
	echo "#include \"grays_marshal.h\"" > $@ && glib-genmarshal $< --prefix=grays_marshal --body > $@

appicon_DATA = pixmaps/gps201_icon.png
applogo_DATA = pixmaps/gps201.png
applogodir = $(pkgdatadir)
appicondir = $(pkgdatadir)

EXTRA_DIST = $(appicon_DATA) gtk-g-rays2.ui grays_marshal.list g-rays2-rfcomm.sh

install-exec-local:
	-xdg-icon-resource install --size 128 $(applogodir)/gps201.png gps-wbt201

