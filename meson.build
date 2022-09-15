project('gtk-g-rays2', 'c', version : '2.1.0')

gnome = import('gnome')

conf_data = configuration_data()
conf_data.set('version', meson.project_version())
configure_file(
  input : 'src/version.h.in',
  output : 'version.h',
  configuration : conf_data
)
configuration_inc = include_directories('build', 'src')
add_project_link_arguments(['-rdynamic'], language: 'c')

subdir('po')
subdir('src')
subdir('data')
subdir('docs')

executable(
    meson.project_name(),
    src,
    dependencies: [
      dependency('gtk+-3.0'),
      dependency('gio-2.0'),
      dependency('glib-2.0'),
      dependency('cairo'),
      dependency('gudev-1.0'),
      dependency('bluez'),
      meson.get_compiler('c').find_library('m')
    ],
    c_args: c_defs,
    include_directories : configuration_inc,
    install: true,
)

mvers = meson.version()
if mvers.version_compare('>=0.58.0')
  gnome.post_install(
    gtk_update_icon_cache: true,
    update_desktop_database: true
  )
else
  meson.add_install_script('meson/post_install.sh')
endif
