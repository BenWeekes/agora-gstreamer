plugin_c_args = ['-DHAVE_CONFIG_H -DBOOB']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'gst-agora')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'agora source and sink pluging')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://www.agora.io')
configure_file(output : 'config.h', configuration : cdata)


# agorasink
agorasink_sources = [
  'agorasink/gstagorasink.c',
  ]

x264 = dependency('x264')
swscale = dependency('libswscale')
opus = dependency('opus')
av= dependency('libavcodec')

gstagorasink = library('gstagorasink',
  agorasink_sources,
  c_args: plugin_c_args,
  dependencies : [gst_dep,x264, swscale, opus, av],
  install : true,
  install_dir : plugins_install_dir,
  objects: [
        '/usr/local/lib/libagorac.so',
        '/usr/local/lib/libagora_rtc_sdk.so',
    ]
)

# agorasrc
agorasrc_sources = [
  'agorasrc/gstagorasrc.c',
  ]

x264 = dependency('x264')
swscale = dependency('libswscale')
opus = dependency('opus')

gst_base_dep = dependency('gstreamer-base-1.0')

gstagorasrc = library('gstagorasrc',
  agorasrc_sources,
  c_args: plugin_c_args,
  dependencies : [gst_dep,x264, swscale, opus,av, gst_base_dep],
  install : true,
  install_dir : plugins_install_dir,
  objects: [
        '/usr/local/lib/libagorac.so',
        '/usr/local/lib/libagora_rtc_sdk.so',
    ]
)

# agoraio
agoraio_sources = [
  'agoraio/gstagoraio.c',
  ]

x264 = dependency('x264')
swscale = dependency('libswscale')
opus = dependency('opus')

gst_base_dep = dependency('gstreamer-base-1.0')
gst_app_dep = dependency('gstreamer-app-1.0')

gstagorio = library('gstagoraio',
  agoraio_sources,
  c_args: plugin_c_args,
  dependencies : [gst_dep,x264, swscale, opus,av,gst_base_dep, gst_app_dep],
  install : true,
  install_dir : plugins_install_dir,
  objects: [
        '/usr/local/lib/libagorac.so',
        '/usr/local/lib/libagora_rtc_sdk.so',
    ]
)

# agoraioudp
agoraioudp_sources = [
  'agoraioudp/gstagoraioudp.c',
  ]

x264 = dependency('x264')
swscale = dependency('libswscale')
opus = dependency('opus')
av= dependency('libavcodec')
gstagorioudp = library('gstagoraioudp',
  agoraioudp_sources,
  c_args: plugin_c_args,
  dependencies : [gst_dep,x264, swscale, opus,av, gst_base_dep, gst_app_dep],
  install : true,
  install_dir : plugins_install_dir,
  objects: [
        '/usr/local/lib/libagorac.so',
        '/usr/local/lib/libagora_rtc_sdk.so',
    ]
)


