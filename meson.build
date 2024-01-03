project('ratify', 'c', version: '2.0.0', license: 'LGPL3+',
    meson_version: '>= 0.50',
    default_options: ['c_std=c99', 'warning_level=2'])

gnome = import('gnome')
pkg = import('pkgconfig')
i18n = import('i18n')

### Versioning #################################################################

version_arr = meson.project_version().split('.')
version_major = version_arr[0].to_int()
version_minor = version_arr[1].to_int()
version_micro = version_arr[2].to_int()

interface_age = version_minor.is_even() ? version_micro : 0
binary_age = 100 * version_minor + version_micro
api_version = '2'

api_name = '@0@-@1@'.format(meson.project_name(), api_version)
gettext_domain = api_name

soversion = 0
libversion = '@0@.@1@.@2@'.format(soversion, binary_age - interface_age, interface_age)

### Paths ######################################################################

includedir = join_paths(get_option('prefix'), get_option('includedir'))
datadir = join_paths(get_option('prefix'), get_option('datadir'))
localedir = join_paths(get_option('prefix'), get_option('localedir'))

### Dependencies ###############################################################

glib = dependency('glib-2.0', version: '>= 2.44')
gio = dependency('gio-2.0', version: '>= 2.44')
gdk_pixbuf = dependency('gdk-pixbuf-2.0', version: '>= 2.6')
gdk = dependency('gdk-3.0')
pango = dependency('pango')
gtk = dependency('gtk+-3.0')

gir = find_program('g-ir-scanner', required: get_option('introspection'))
vapigen = find_program('vapigen', required: get_option('vapi'))
gtkdoc = find_program('gtkdoc-scan', required : get_option('gtk_doc'))

### Header Configuration #######################################################

config_h = configuration_data()
config_h.set_quoted('PACKAGE_NAME', meson.project_name())
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', gettext_domain)
configure_file(output: 'config.h', configuration: config_h)

### Compiler Configuration #####################################################

common_cflags = []
common_ldflags = []

host_system = host_machine.system()
cc = meson.get_compiler('c')

# Compiler flags
# Justification for -Wno-missing-field-initializers: We make extensive use of
# partially initializing structs, and the code for all the control word tables
# would be quite verbose if we didn't.
# Justification for -Wno-cast-function-types: GLib's callback-style code needs
# to be able to cast functions from one signature to another, and GLib requires
# that your compiler be able to do this.
# Justification for -Wno-unused-parameter: GLib's callback-style code has many
# callback types with certain signatures, and more often than not we don't need
# to use all the parameters passed to a callback.
common_cflags += cc.get_supported_arguments([
    '-Wno-missing-field-initializers',
    '-Wno-cast-function-type',
    '-Wno-unused-parameter',
])

# Maintain compatibility with autotools
if host_system == 'darwin'
  common_ldflags += [
    '-compatibility_version 1',
    '-current_version @0@.@1@'.format(binary_age - interface_age, interface_age),
  ]
endif

root_dir = include_directories('.')

### C Library and Headers ######################################################

public_headers = [
    'ratify/rtf.h',
]

introspection_sources = [
    'ratify/rtf.c',
]

sources = introspection_sources + [
    'ratify/init.c',
    'ratify/rtf-colortbl.c',
    'ratify/rtf-deserialize.c',
    'ratify/rtf-document.c',
    'ratify/rtf-field.c',
    'ratify/rtf-fonttbl.c',
    'ratify/rtf-footnote.c',
    'ratify/rtf-ignore.c',
    'ratify/rtf-langcode.c',
    'ratify/rtf-picture.c',
    'ratify/rtf-serialize.c',
    'ratify/rtf-state.c',
    'ratify/rtf-stylesheet.c',
]

library_cflags = [
    '-DG_LOG_DOMAIN="@0@"'.format(meson.project_name()),
    '-DLOCALEDIR="@0@"'.format(localedir),
]

libratify = library(api_name, sources,
    version: libversion, soversion: soversion,
    include_directories: root_dir,
    dependencies: [glib, gio, gdk_pixbuf, gdk, pango, gtk],
    c_args: library_cflags + common_cflags,
    link_args: common_ldflags,
    gnu_symbol_visibility: 'hidden',
    install: true)

install_headers(public_headers, subdir: join_paths(api_name, 'ratify'))

libratify_dep = declare_dependency(include_directories: root_dir,
    link_with: libratify,
    dependencies: [glib, gio, gdk_pixbuf, gdk, pango, gtk])

### Pkgconfig File #############################################################

pkg.generate(libratify, subdirs: api_name, filebase: api_name,
    requires: [glib, gio, gdk, gtk], requires_private: [gdk_pixbuf, pango],
    name: 'Ratify',
    description: 'Library for importing and exporting RTF documents in GTK',
    url: 'https://github.com/ptomato/ratify')

### GObject Introspection Bindings #############################################

if gir.found()
    gir_file = gnome.generate_gir(libratify,
        sources: public_headers + introspection_sources,
        namespace: 'Ratify',
        nsversion: api_version,
        identifier_prefix: 'Rtf',
        symbol_prefix: 'rtf',
        export_packages: api_name,
        includes: ['Gtk-3.0'],
        header: 'ratify/rtf.h',
        install: true,
        extra_args: ['--quiet'])

    if vapigen.found()
        gnome.generate_vapi(api_name, sources: gir_file[0],
            packages: ['gtk+-3.0'],
            install: true)
    endif
endif

### Documentation ##############################################################

subdir('docs/reference')

### Translation ################################################################

subdir('po')

### Tests ######################################################################

test_cflags = [
    '-DTESTFILEDIR="@0@"'.format(join_paths(meson.current_source_dir(), 'testcases')),
]

run_tests = executable('run-tests', 'testcases/test.c',
    c_args: test_cflags + common_cflags, dependencies: libratify_dep)

test('tests', run_tests, args: '--tap', protocol: 'tap',
    env: ['GNOME_DISABLE_CRASH_DIALOG=1', 'NO_AT_BRIDGE=1'])

add_test_setup('human',
    exe_wrapper: ['gtester', '-k', '-m=thorough'],
    timeout_multiplier: 1000)

### Output Summary #############################################################

summary = [
  '',
  '--------',
  'Building @0@ @1@'.format(meson.project_name(), meson.project_version()),
  ' Introspection: @0@'.format(gir.found()),
  ' Documentation: @0@'.format(gtkdoc.found()),
  '     VAPI file: @0@'.format(gir.found() and vapigen.found())
]

message('\n'.join(summary))
