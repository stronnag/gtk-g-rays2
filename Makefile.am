SUBDIRS = src po data
ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = dist-xz

EXTRA_DIST = \
	data \
	docs \
	src/pixmaps \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	autogen.sh

install-data-local:
	@$(mkinstalldirs) $(DESTDIR)$(docdir)
	@$(mkinstalldirs) $(DESTDIR)$(datarootdir)/help/C/gtk-g-rays2
	@for f in docs/*.png docs/*.page docs/*.html ; do \
	 $(INSTALL_DATA) $$f $(DESTDIR)$(datarootdir)/help/C/gtk-g-rays2/ ; \
	done
	@for f in docs/README docs/*.txt ; do \
	 $(INSTALL_DATA) $$f $(DESTDIR)$(docdir) ; \
        done
	-gtk-update-icon-cache /usr/share/icons/hicolor