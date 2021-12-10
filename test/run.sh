cd ..
./build_all.sh
cd test
GST_PLUGIN_PATH=/usr/local/lib/x86_64-linux-gnu/gstreamer-1.0 GST_DEBUG="3,GST_TRACER:7" GST_TRACERS="leaks(name=all-leaks)" ./test_call_end
