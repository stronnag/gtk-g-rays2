src= files('babel_spawn.c', 'messages.c', 'serial.c', 'wbt201.c', 'cairo_draw.c', 'misc.c', 'uihandler.c', 'bluecon.c')

localedir = join_paths(get_option('prefix'), 'locale')
package_data = join_paths(get_option('prefix'), 'share', 'gtk-g-rays2')
help_dir = join_paths(get_option('prefix'), 'share', 'help', 'C', 'gtk-g-rays2')

c_defs = ['-D_GNU_SOURCE', '-DLOCALEDIR="'+localedir+'"',
	  '-DPACKAGE_DATA_DIR="'+package_data+'"',
	  '-DHELPDIR="'+help_dir+'"']

deps = [
  dependency('gtk+-3.0'),
  dependency('gio-2.0'),
  dependency('glib-2.0'),
  dependency('cairo'),
  meson.get_compiler('c').find_library('m')
]

if host_machine.system() == 'linux'
  deps += dependency('gudev-1.0')
  deps += dependency('bluez')
endif

mlist = files('grays_marshal.list')
m = gnome.genmarshal('grays_marshal',
		     sources: mlist,
		     install_header: false,
		     internal: true,
		     stdinc: true,
		     valist_marshallers: true,
		    )
src += m
add_project_link_arguments(['-rdynamic'], language: 'c')
