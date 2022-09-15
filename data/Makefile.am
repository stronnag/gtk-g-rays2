pixmapdir=$(datadir)/icons/hicolor/48x48/apps
gnomapdir = $(datadir)/applications
gnomap_DATA=gtk-g-rays2.desktop
pixmap_DATA=gtk-g-rays2.png	

gtk_update_icon_cache = gtk-update-icon-cache -f $(datadir)/icons/hicolor

install-data-hook:
	@-if test -z "$(DESTDIR)"; then					\
		$(gtk_update_icon_cache);				\
	else								\
		echo "*** Icon cache not updated. Remember to run:";	\
		echo "***";						\
		echo "***   $(gtk_update_icon_cache)";			\
	echo "***";						\
	fi
