library_src_dir = join_paths(meson.current_source_dir(), '..', '..',
    meson.project_name())

private_headers = [
    'init.h',
    'rtf-deserialize.h',
    'rtf-document.h',
    'rtf-ignore.h',
    'rtf-langcode.h',
    'rtf-serialize.h',
    'rtf-state.h',
]

version_xml = configure_file(input: 'version.xml.in', output: 'version.xml',
    configuration: {'PACKAGE_VERSION': meson.project_version()})

if gtkdoc.found()
    gnome.gtkdoc(api_name,
        main_xml: '@0@-docs.sgml'.format(meson.project_name()),
        src_dir: library_src_dir,
        content_files: version_xml,
        dependencies: libratify_dep,
        ignore_headers: private_headers,
        gobject_typesfile: 'ratify.types',
        mkdb_args: ['--sgml-mode', '--output-format=xml'],
        install: true)
endif
