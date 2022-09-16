install_data(
  'gtk-g-rays2.ui', 'g-rays2-rfcomm.sh', 'pixmaps/gps201_icon.png', 'pixmaps/gps201.png',
  install_dir: join_paths(get_option('datadir'), 'gtk-g-rays2')
)

icon_sizes = ['48', '64', '128']
foreach i : icon_sizes
  ixi = i+'x'+i
  install_data (join_paths('icons',ixi,'gtk-g-rays2.png'),
		 install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', ixi, 'apps')
    )
endforeach

install_data (join_paths('icons', '128x128', 'gtk-g-rays2.png'),
	      install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', 'scalable', 'apps'))

install_data('gtk-g-rays2.desktop',
	     install_dir : get_option('datadir') / 'applications')
