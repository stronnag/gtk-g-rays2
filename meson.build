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

subdir('po')
subdir('src')
subdir('data')
subdir('docs')

executable(
    meson.project_name(),
    src,
    dependencies: deps,
    c_args: c_defs,
    include_directories : configuration_inc,
    install: true,
)

mvers = meson.version()
if mvers.version_compare('>=0.59.0')
  if host_machine.system() == 'darwin'
    gnome.post_install(
      gtk_update_icon_cache: true,
    )
  else
    gnome.post_install(
      gtk_update_icon_cache: true,
      update_desktop_database: true
    )
  endif
else
  meson.add_install_script('meson/post_install.sh')
endif
