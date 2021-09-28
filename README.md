A gstreamer wrapper for Agora Linux SDK (sink and src)


## install gstreamer

sudo apt-get --fix-broken --fix-missing install  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio

## requireed libraries:

sudo apt install meson
sudo apt-get install -y libswscale-dev
sudo apt-get install -y x264

## build and install libagorac library

$ cd agora-gstreamer/agorasink/libagorac
$ sudo ./install.sh ~/agora_rtc_sdk

## build the plugin

$ cd agora-gstreamer/agorasink
$ meson build
$ ./c

## run and test

$ export GST_PLUGIN_PATH=/usr/local/lib/x86_64-linux-gnu/gstreamer-1.0
$ gst-launch-1.0 -v videotestsrc ! x264enc ! agorasink