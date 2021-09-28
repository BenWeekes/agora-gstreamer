plugin_c_args = ['-DHAVE_CONFIG_H']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'gst-agorasink')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'GStreamer template Plug-ins')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://gstreamer.freedesktop.org')
configure_file(output : 'config.h', configuration : cdata)


# Plugin 3 (agorasink)
agorasink_sources = [
  'src/gstagorasink.c',
  'src/agorah264parser.c',
  ]

x264 = dependency('x264')
swscale = dependency('libswscale')
opus = dependency('opus')

gstagorasink = library('gstagorasink',
  agorasink_sources,
  c_args: plugin_c_args,
  dependencies : [gst_dep,x264, swscale, opus],
  install : true,
  install_dir : plugins_install_dir,
  objects: [
        '/usr/local/lib/libagorac.so',
        '/usr/local/lib/libagora_rtc_sdk.so',
        '/usr/local/lib/libagora-ffmpeg.so',
    ]
)
