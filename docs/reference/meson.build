doc_lib3270 = lib3270
doc_lib3270_glib = lib3270_glib
if get_option('default_library') == 'both'
  doc_lib3270 = lib3270.get_shared_lib()
  doc_lib3270_glib = lib3270_glib.get_shared_lib()
endif

doc_dep = declare_dependency(
  link_with: [doc_lib3270_glib, doc_lib3270],
  include_directories: includes_dir,
  dependencies: [gobject, polkit],
)

fs = import('fs')
gnome = import('gnome')

fixxref_args = [
  '--html-dir=@0@'.format(join_paths(get_option('prefix'), get_option('datadir'), 'gtk-doc', 'html')),
]

glib_prefix = gobject.get_variable(pkgconfig: 'prefix', default_value: '/usr')
foreach book : ['glib', 'gobject', 'gio']
  extra_dir = glib_prefix / 'share' / 'gtk-doc' / 'html' / book
  if fs.exists(extra_dir)
    fixxref_args += ['--extra-dir=@0@'.format(extra_dir)]
  endif
endforeach

gnome.gtkdoc(
  'lib3270',
  module_version: libversion,
  src_dir: [
    meson.project_source_root() / 'src/include/lib3270',
    meson.project_source_root() / 'src/gobject',
  ],
  main_xml: 'lib3270-docs.xml',
  gobject_typesfile: 'lib3270.types',
  dependencies: doc_dep,
  include_directories: includes_dir,
  ignore_headers: [
    'actions.h',
    'charset.h',
    'contents.h',
    'defs.h',
    'internals.h',
    'keyboard.h',
    'log.h',
    'mainloop.h',
    'memory.h',
    'popup.h',
    'properties.h',
    'selection.h',
    'session.h',
    'ssl.h',
    'toggle.h',
    'trace.h',
  ],
  scan_args: [
    '--ignore-decorators=LIB3270_EXPORT',
    '--ignore-decorators=LIB3270_INTERNAL',
  ],
  mkdb_args: [
    '--ignore-files=actions.h charset.h contents.h defs.h internals.h keyboard.h log.h mainloop.h memory.h popup.h properties.h selection.h session.h ssl.h toggle.h trace.h',
  ],
  fixxref_args: fixxref_args,
  install: true,
)
