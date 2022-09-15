#!/bin/sh

if [ -z $DESTDIR ]; then
  echo >&2 Updating desktop icon cache ...
  gtk-update-icon-cache -qtf $MESON_INSTALL_PREFIX/share/icons/hicolor

  echo >&2  Updating desktop database ...
  update-desktop-database  $MESON_INSTALL_PREFIX/share/applications
fi
