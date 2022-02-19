cc test_signals.c  `pkg-config --cflags --libs gstreamer-1.0` -o test_signals
cc test_call_end.c `pkg-config --cflags --libs gstreamer-1.0` -o test_call_end
cc test_sync_buffer.c `pkg-config --cflags --libs gstreamer-1.0` -o test_sync_buffer
cc test_publish_unpublish.c `pkg-config --cflags --libs gstreamer-1.0` -o test_publish_unpublish

cc test_agora_sink.c `pkg-config --cflags --libs gstreamer-1.0` -o test_agora_sink
cc test_pause_resume.c `pkg-config --cflags --libs gstreamer-1.0` -o test_pause_resume

cc test_arm.c `pkg-config --cflags --libs gstreamer-1.0` -o test_arm
