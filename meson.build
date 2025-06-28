# Maintainer: Perry Werneck <perry.werneck@gmail.com>

# SPDX-License-Identifier: LGPL-3.0-or-later 

# Copyright (C) 2024 Banco do Brasil S. A.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

project(
	'lib3270', 
	['c'],
	version: '5.6.0',
	license: 'GPL-3.0-or-later',
)

project_description = 'TN3270 client library'
product_name = 'pw3270'
product_domain = 'br.app.@0@'.format(product_name)
product_path = '/br/app/@0@'.format(product_name)
product_title = 'TN3270 Access library'

lib_src = []

#
# Versioning
#
pkg_version = meson.project_version()
version_array = pkg_version.split('.')
pkg_major_version = version_array[0].to_int()
pkg_minor_version = version_array[1].to_int()
pkg_micro_version = version_array[2].to_int()

libversion = '@0@.@1@'.format(pkg_major_version,pkg_minor_version)

#
# i18n
#
subdir('po')

#
# Dependencies
#
libssl = dependency('OpenSSL', required: false)
libintl = dependency('intl', required: false)
libiconv = dependency('iconv', required: false)
gobject = dependency('gobject-2.0', required: false)
polkit = dependency('polkit-gobject-1', required: false)

lib_extra = [
  libintl,
  libiconv
]

lib_deps = [
  dependency('liburiparser', method : 'pkg-config'),
  libssl,
]

#
# Setup time based properties
#
time_epoch = run_command('sh', '-c', 'echo "$SOURCE_DATE_EPOCH"', check : true).stdout().strip()
if time_epoch == ''
  time_epoch = run_command('date','+%Y%m%d', check: true).stdout().strip()
  datecmd = 'date'
else
  datecmd = 'date -u -d@' + time_epoch
endif

#
# Compiler flags
#
c = meson.get_compiler('c')
compiler_flags = [
  '-ggdb3',
  '-pthread',
  '-DHAVE_CONFIG_H=1',
  '-DBUILD_DATE='+time_epoch,
  '-DG_LOG_DOMAIN="tn3270"',
]

# TODO:
# https://github.com/PerryWerneck/lib3270/pull/59#discussion_r1923066195
# 
# You may be interested in meson's builtin option b_ndebug=if-release:
# https://mesonbuild.com/Builtin-options.html#base-options
#
enable_debug = get_option('debug') or get_option('buildtype').contains('debug')
if enable_debug
  compiler_flags += [
	  '-DDEBUG=1'
  ]
else
  compiler_flags += [
	'-DNDEBUG=1'
  ]
endif

add_project_arguments(c.get_supported_arguments(compiler_flags), language: 'c')

#
# Configuration
#
app_conf = configuration_data()
app_conf.set_quoted('PACKAGE_NAME', meson.project_name())
app_conf.set_quoted('PACKAGE_VERSION', meson.project_version())
app_conf.set_quoted('PACKAGE_DESCRIPTION', project_description)
app_conf.set('PACKAGE_VERSION_MAJOR', pkg_major_version)
app_conf.set('PACKAGE_VERSION_MINOR', pkg_minor_version)
app_conf.set('PACKAGE_VERSION_MICRO', pkg_micro_version)

app_conf.set('UNLOCK_MS', 350)

if host_machine.system() != 'windows'

  app_conf.set('LIBDIR', 
    '@0@/@1@'.format(
      get_option('prefix'),
      get_option('libdir')
    )
  )

  app_conf.set('DATADIR', 
    '@0@/@1@/@2@'.format(
      get_option('prefix'),
      get_option('datadir'),
      product_name
    )
  )

  app_conf.set('HAVE_LIBCURL', 1)
  lib_deps += [
    dependency('libcurl'),  
  ]

endif

if libiconv.found()
  app_conf.set('HAVE_ICONV', 1)
endif

app_conf.set('HAVE_STRPTIME', c.has_function('strptime'))
app_conf.set('HAVE_STRCASESTR', c.has_function('strcasestr'))
app_conf.set('HAVE_LOCALTIME_R', c.has_function('localtime_r'))

app_conf.set('HAVE_STRTOK_R', c.has_function('strtok_r'))
app_conf.set('HAVE_VASPRINTF', c.has_function('vasprintf'))

app_conf.set_quoted('PRODUCT_TITLE', product_title)

app_conf.set('PRODUCT_NAME',product_name)
app_conf.set('PRODUCT_DOMAIN',product_domain)
app_conf.set('PRODUCT_PATH',product_path)

# Fix-me
app_conf.set('HAVE_GNUC_VISIBILITY', 1)
app_conf.set('HAVE_GETADDRINFO', 1)
app_conf.set('HAVE_INET_NTOP', 1)

# Legacy
app_conf.set('X3270_TN3270E', 1)
app_conf.set('X3270_ANSI', 1)
app_conf.set('X3270_APL', 1)
app_conf.set('X3270_TRACE', 1)
app_conf.set('X3270_FT', 1)
#app_conf.set('SSL_ENABLE_SELF_SIGNED_CERT_CHECK', 1)
#app_conf.set('SSL_ENABLE_CRL_CHECK', 1)
#app_conf.set('SSL_ENABLE_CRL_EXPIRATION_CHECK', 1)

package_release = run_command('sh', '-c', datecmd + ' +%-y.%-m.%-d', check : true).stdout().strip()
app_conf.set_quoted('PACKAGE_RELEASE',package_release)

rpq_build_date = run_command('sh', '-c', 'LANG=C ' + datecmd, check : true).stdout().strip()
app_conf.set_quoted('RPQ_BUILD_DATE', rpq_build_date)

rpq_timestamp_value = run_command('sh', '-c', datecmd + ' +%Y%m%d%H%M%S', check : true).stdout().strip()
app_conf.set_quoted('RPQ_TIMESTAMP_VALUE', rpq_timestamp_value)

rpq_revision= run_command('sh', '-c', datecmd + ' +%Y%m%d', check : true).stdout().strip()
app_conf.set_quoted('RPQ_REVISION', rpq_revision)

sccs_user= run_command('sh', '-c', 'echo "$USER"', check : true).stdout().strip()
app_conf.set_quoted('SCCS_USER', sccs_user)

sccs_date = run_command('sh', '-c', datecmd + ' +%Y/%m/%d', check : true).stdout().strip()
app_conf.set_quoted('SCCS_DATE', sccs_date)

if c.compiles('#include <unistd.h>', name : 'unistd.h')
  app_conf.set('HAVE_UNISTD_H', 1)
endif

if c.compiles('#include <linux/limits.h>', name : 'limits.h')
  app_conf.set('HAVE_LINUX_LIMITS_H', 1)
endif

if c.compiles('#include <signal.h>', name : 'signal.h')
  app_conf.set('HAVE_SIGNAL_H', 1)
endif

if c.compiles('#include <syslog.h>', name : 'syslog.h')
  app_conf.set('HAVE_SYSLOG', 1)
endif

if libintl.found()
  app_conf.set('HAVE_LIBINTL', 1)
endif

if c.compiles('#include <malloc.h>', name : 'malloc.h')
  app_conf.set('HAVE_MALLOC_H', 1)
endif

#
# Sources
#
lib_src += [
  'src/library/actions/actions.c',
  'src/library/actions/table.c',
  'src/library/ansi.c',
  'src/library/array.c',
  'src/library/bounds.c',
  'src/library/charset/charset.c',
  'src/library/charset/convert.c',
  'src/library/charset/getset.c',
  'src/library/charset/iconv.c',
  'src/library/charset/remap.c',
  'src/library/charset/utf8.c',
  'src/library/charset/view.c',
  'src/library/connect.c',
  'src/library/ctlr.c',
  'src/library/cursor.c',
  'src/library/field.c',
  'src/library/ft/ft.c',
  'src/library/ft/ft_cut.c',
  'src/library/ft/ft_dft.c',
  'src/library/ft/ftmessages.c',
  'src/library/ft/set.c',
  'src/library/host.c',
  'src/library/html.c',
  'src/library/init.c',
  'src/library/keyboard/actions.c',
  'src/library/keyboard/kybd.c',
  'src/library/keyboard/properties.c',
  'src/library/linkedlist.c',
  'src/library/log.c',
  'src/library/model.c',
  'src/library/options.c',
  'src/library/paste.c',
  'src/library/popup.c',
  'src/library/print.c',
  'src/library/printer.c',
  'src/library/properties/boolean.c',
  'src/library/properties/get.c',
  'src/library/properties/signed.c',
  'src/library/properties/string.c',
  'src/library/properties/unsigned.c',
  'src/library/resources.c',
  'src/library/rpq.c',
  'src/library/screen.c',
  'src/library/see.c',
  'src/library/session.c',
  'src/library/sf.c',
  'src/library/state.c',
  'src/library/telnet.c',
  'src/library/toggles/getset.c',
  'src/library/toggles/init.c',
  'src/library/toggles/listener.c',
  'src/library/toggles/table.c',
  'src/library/trace_ds.c',
  'src/library/util.c',
  'src/library/wait.c',
  'src/library/selection/actions.c',
  'src/library/selection/get.c',
  'src/library/selection/selection.c',
  'src/library/network/state.c',
  'src/library/network/tools.c',
  'src/library/os/' + host_machine.system() + '/resolv.c',
  'src/library/os/' + host_machine.system() + '/connect.c',
  'src/library/os/' + host_machine.system() + '/insecure.c',
  'src/library/os/' + host_machine.system() + '/mainloop.c',
  'src/library/os/' + host_machine.system() + '/log.c',

]

gobject_src = [
  'src/gobject/object.c',
  'src/gobject/callbacks.c',
]

linux_src =[
  'src/library/os/linux/util.c',
  'src/library/os/linux/glib.c',
] 

darwin_src = [
  'src/library/os/darwin/connect.c',
  'src/library/os/darwin/curl.c',
  'src/library/os/darwin/download.c',
  'src/library/os/darwin/ldap.c',
  'src/library/os/darwin/util.c',
]

win_src = [
  'src/library/os/windows/connect.c',
  'src/library/os/windows/handle.c',
  'src/library/os/windows/registry.c',
  'src/library/os/windows/util.c',
  'src/library/os/windows/eventhandler.c',
]

#
# SDK
# https://mesonbuild.com/Pkgconfig-module.html
#
pkg = import('pkgconfig')

pkg_variables = [

  'version_major=@0@'.format(pkg_major_version),
  'version_minor=@0@'.format(pkg_minor_version),
  'version_micro=@0@'.format(pkg_micro_version),

  'product_name=@0@'.format(product_name),
  'product_id=@0@.@1@'.format(product_domain,product_name),
  'product_title=@0@'.format(product_title),
  'product_version=@0@@1@'.format(pkg_major_version,pkg_minor_version),

  'build_id=@0@'.format(rpq_timestamp_value),

  'package_name=lib3270-@0@_@1@'.format(pkg_major_version,pkg_minor_version),
  
  'sdk_version=@0@.@1@'.format(pkg_major_version,pkg_minor_version),
  'library_name=lib3270',
  'product_domain=@0@'.format(product_domain),
  'schema_path=@0@'.format(product_path),

  'plugin_path=${libdir}/@0@/@1@.@2@/plugins/'.format(
    product_name,pkg_major_version,pkg_minor_version,
  ),
  'module_version=@0@.@1@'.format(pkg_major_version,pkg_minor_version),

]

if host_machine.system() == 'windows'

  pkg.generate(
    name: meson.project_name(),
    description: project_description,
    variables: pkg_variables,
    libraries: [ '-l3270.dll' ],
    requires_private: [ 
      lib_deps 
    ],
    libraries_private: [ 
      '-l3270',
      '-lwinhttp',
      '-lws2_32',
      '-lwtsapi32',
      '-lcomdlg32',
      '-lSecur32',
    ]
  )

  pkg.generate(
    name: meson.project_name() + '-static',
    description: project_description,
    variables: pkg_variables,
    requires: [
      lib_deps
    ],
    libraries: [ 
      '-l3270',
      '-lwinhttp',
      '-lws2_32',
      '-lwtsapi32',
      '-lcomdlg32' 
    ]
  )

  pkg.generate(
    name: meson.project_name() + '-delayed',
    description: project_description,
    variables: pkg_variables,
    libraries: [ 
      '-l3270.delayed',
    ]
  )

else

  pkg.generate(
    name: meson.project_name(),
    description: project_description,
    variables: pkg_variables,
    libraries: [ '-l3270' ],
    requires_private: [ lib_deps ],
    libraries_private: [ lib_deps ],
  )

  pkg.generate(
    name: meson.project_name() + '-static',
    description: project_description,
    variables: pkg_variables,
    requires: [
      lib_deps
    ],
    libraries: [ '-l:' + meson.project_name() + '.a' ]
  )


endif

# Check for OSs
# https://mesonbuild.com/Reference-manual_builtin_host_machine.html

includes_dir = include_directories(
  'src/include',
  'src/include/' + host_machine.system(),
)

if host_machine.system() == 'windows'

  app_conf.set_quoted('LIBEXT', '.dll')
  app_conf.set('SECURITY_WIN32',1)

  lib_src += win_src
  lib_src += [
    'src/library/tls/wsc/start.c',
  ]

endif

if host_machine.system() == 'linux'

  app_conf.set_quoted('LIBEXT', '.so')

  lib_src += linux_src

  if polkit.found()

    app_conf.set('HAVE_POLKIT', 1)

  endif

  if libssl.found()

    app_conf.set('HAVE_OPENSSL', 1)

    lib_src += [
      'src/library/tls/openssl/start.c',
      'src/library/tls/openssl/context.c',
      'src/library/tls/openssl/connected.c',
      'src/library/tls/openssl/errors.c',
      'src/library/tls/openssl/messages.c',
      'src/library/tls/openssl/fqdn.c',
    ]

  else 

    lib_src += [
      'src/library/tls/unavailable.c',
    ]

  endif

endif

if host_machine.system() == 'darwin'

  includes_dir = include_directories(
    'src/include',
    'src/include/darwin',
  )

  install_headers(
    'src/include/darwin/lib3270/os.h',
    subdir: 'lib3270'  
  )

  lib_extra += [ dependency('appleframeworks', modules : 'CoreFoundation') ]
  lib_src += darwin_src

  if libssl.found()

    app_conf.set('HAVE_OPENSSL', 1)

    lib_src += [
      'src/library/tls/openssl/context.c',
      'src/library/tls/openssl/init.c',
      'src/library/tls/openssl/errors.c',
    ]

  else 

    lib_src += [
      'src/library/tls/unavailable.c',
    ]

  endif

endif

#
# Targets
#
config_src = [
  configure_file(
      output : 'config.h', 
      configuration : app_conf
  )
]

#hotdoc = import('hotdoc')
#hotdoc.generate_doc(
#  meson.project_name(),
#  project_version: meson.project_version(),
#  sitemap: 'sitemap.txt',
#  index: 'index.md',
#  c_sources: lib_src,
#  c_smart_index: true,
#  install: true,
#)

if host_machine.system() == 'windows'

  # https://mesonbuild.com/Windows-module.html
  windows = import('windows')
  resources = windows.compile_resources(
    configure_file(
      input : 'src/library/os/windows/resources.rc.in',
      output : 'resources.rc',
      configuration : app_conf
    )
  )

  dynamic = shared_library(
    '3270', 
    config_src + lib_src + resources,
    install: true,
    dependencies: lib_deps + lib_extra,
    include_directories: includes_dir,
    link_args: [
      '-lwinhttp',
      '-lws2_32',
      '-lwtsapi32',
      '-lcomdlg32',
      '-lsecur32',
      '-lcrypt32',
      '-Wl,--output-def,lib3270.def',
    ],
  )

  #
  # https://baltig.polito.it/s346827/qemu/-/blob/36025b9c539f2c25f77bd985adc47825b9e9c4f4/plugins/meson.build
  # https://github.com/mesonbuild/meson/issues/5093
  #
  if meson.is_cross_build()
    dlltool = find_program(
      '@0@/bin/dlltool'.format(
        meson.get_external_property('root')
      )
    )
  else
    dlltool = find_program('dlltool', required: true)
  endif

  # https://mesonbuild.com/Reference-manual_functions.html#custom_target
  custom_target(
    'delaylib',
    depends: dynamic,
    build_by_default: true,
    command: [ 
      dlltool,
        '--input-def','lib3270.def',
        '--dllname','lib3270.dll',
        '--kill-at',
        '--output-lib','lib3270.delayed.a' 
      ],
    output: 'lib3270.delayed.a',
    install: true,
    install_tag: 'devel',
    install_dir: get_option('libdir')
  )

  static = static_library(
    '3270', 
    config_src + lib_src,
    pic: true,
    c_args: [ '-DSTATIC_LIBRARY' ],
    install: true,
    dependencies: lib_deps,
    include_directories: includes_dir
  )

elif host_machine.system() == 'darwin'

  dynamic = shared_library(
    '3270', 
    config_src + lib_src,
    install: true,
    version : libversion,
    soversion : libversion,
    gnu_symbol_visibility: 'hidden',
    dependencies: lib_deps + lib_extra,
    include_directories: includes_dir
  )

  static = static_library(
    '3270', 
    config_src + lib_src,
    pic: true,
    c_args: [ '-DSTATIC_LIBRARY' ],
    install: true,
    dependencies: lib_deps,
    include_directories: includes_dir
  )

else

  dynamic = shared_library(
    '3270', 
    config_src + lib_src,
    install: true,
    version : libversion,
    soversion : libversion,
    c_args: [ '-ffat-lto-objects' ],
    gnu_symbol_visibility: 'hidden',
    dependencies: lib_deps + lib_extra,
    include_directories: includes_dir
  )

  static = static_library(
    '3270', 
    config_src + lib_src,
    pic: true,
    c_args: [ '-DSTATIC_LIBRARY', '-fno-lto' ],
    install: true,
    dependencies: lib_deps,
    include_directories: includes_dir
  )

endif

glib_dynamic = shared_library(
    '3270-glib', 
    gobject_src,
    install: true,
    version : libversion,
    soversion : libversion,
    link_with : [ dynamic ],
    dependencies: lib_deps + lib_extra + [ gobject, polkit ],
    include_directories: includes_dir
)

dynamic_library = declare_dependency(
  link_with : dynamic,
  include_directories : includes_dir,
  variables: pkg_variables,
) 

static_library = declare_dependency(
  link_with : static,
  include_directories : includes_dir,
  variables: pkg_variables,
) 

static_library(
  '3270-glib', 
  gobject_src + config_src + lib_src,
  install: true,
  dependencies: lib_deps + [ gobject, polkit ],
  include_directories: includes_dir
)

executable(
    meson.project_name() + '-gobject',
    [ 'src/gobject/testprogram.c' ],
    install: false,
    link_with : [ glib_dynamic, dynamic ],
    dependencies: lib_deps + [ gobject ],
    include_directories: includes_dir
)

executable(
  meson.project_name(),
  config_src + [ 'src/library/testprogram.c' ],
  install: false,
  link_with : [ dynamic ],
  dependencies: lib_deps + libintl,
  include_directories: includes_dir
)

install_headers(
  'src/include/lib3270.h',
)

install_headers(
  'src/include/lib3270/actions.h',
  'src/include/lib3270/charset.h',
  'src/include/lib3270/filetransfer.h',
  'src/include/lib3270/memory.h',
  'src/include/lib3270.h',
  'src/include/lib3270/keyboard.h',
  'src/include/lib3270/log.h',
  'src/include/lib3270/popup.h',
  'src/include/lib3270/properties.h',
  'src/include/lib3270/selection.h',
  'src/include/lib3270/session.h',
  'src/include/lib3270/ssl.h',
  'src/include/lib3270/toggle.h',
  'src/include/lib3270/trace.h',
  'src/include/' + host_machine.system() + '/lib3270/os.h',
  subdir: 'lib3270'  
)

