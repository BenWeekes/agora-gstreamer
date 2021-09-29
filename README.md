A gstreamer wrapper for Agora Linux SDK (sink and src)


## Install gstreamer and dependencies
   sudo apt-get update     
   sudo apt-get --fix-broken --fix-missing install  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio   

## Install additional libraries:

   sudo apt-get install -y meson libswscale-dev x264    

## Build and install libagorac library

   wget https://download.agora.io/sdk/release/Agora-RTC-x86_64-linux-gnu-v3.4.217.tgz   
   tar -xvzf Agora-RTC-x86_64-linux-gnu-v3.4.217.tgz   

   cd ~/agora-gstreamer/agorasink/libagorac   
   sudo ./install.sh ~/agora_rtc_sdk   

## Build this plugin

   cd ~/agora-gstreamer/agorasink   
   meson build   
   ./c   

## Run and test

   export GST_PLUGIN_PATH=/usr/local/lib/x86_64-linux-gnu/gstreamer-1.0   
   
   gst-launch-1.0 -v videotestsrc ! x264enc ! agorasink   
   gst-launch-1.0 -v videotestsrc is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1   ! videoconvert ! x264enc key-int-max=20 ! agorasink   
   gst-launch-1.0 v4l2src ! jpegdec ! videoconvert ! x264enc key-int-max=20 ! agorasink   

   
