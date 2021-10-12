A gstreamer wrapper for Agora Linux SDK (sink and src)


## Install gstreamer and dependencies
   sudo apt-get update     
   sudo apt-get --fix-broken --fix-missing install -y libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio   

## Install additional libraries:

   sudo apt-get install -y meson libswscale-dev x264 libx264-dev libopus-dev   
   sudo apt install -y build-essential git libpcre3 libpcre3-dev zlib1g zlib1g-dev libssl-dev unzip     
   sudo apt install -y libavcodec-dev libavformat-dev libavutil-dev nasm libavfilter-dev libopus-dev   
 
## Build and install 
   After installing the libraries above on your Ubuntu system         
   Clone this repo then      
   cd agora-gstreamer     
  ./build_all.sh
  
  If no errors are printed the new agora gs plugins will be installed on the system ready for use


## creating and installing a binary release:

To create a binary release:

cd release
./make-release

To install the release on the target machine:

cd release
./install


## Pipeline Configuration Properties

 appid -- sets agora app id
 
 channel  -- sets agora channel id

 userid   -- sets agora user id (optional)

 silent -- a flag to show/hide debug info

## Run and test

   export GST_PLUGIN_PATH=/usr/local/lib/x86_64-linux-gnu/gstreamer-1.0   
   
 ## agorasink
   
Video into Agora from test source:    

gst-launch-1.0 -v videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1   ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agorasink appid=xxx channel=test silent=1

Video into Agora from webcam source:     

 gst-launch-1.0 v4l2src ! jpegdec ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agorasink appid=xxx channel=test silent=1
 
Audio into Agora from test source:    

gst-launch-1.0 -v audiotestsrc wave=sine ! audioconvert ! opusenc ! agorasink audio=true appid=xxx channel=test silent=1


 ## agorasrc

Video out of Agora:    
   agorasrc can be used to read encoded h264 from an agora channel, here is an example pipleline:     
   
   gst-launch-1.0 agorasrc verbose=false appid=xxx channel=xxx userid=xxx ! decodebin ! glimagesink     

   gst-launch-1.0 agorasrc verbose=false  appid=xxx channel=xxx userid=xxx ! decodebin ! autovideosink      

   where appid and channel is same as agorasink. The value of userid represents which user agorasrc should subscribe to    
   
 
 Audio out of Agora
 
   gst-launch-1.0 agorasrc audio=true verbose=false appid=xxx channel=gstreamer userid=xxx ! filesink location=test.raw     
   
   gst-launch-1.0 agorasrc audio=true verbose=false appid=xxx channel=xxx userid=xxx ! audio/x-raw,format=S16LE,channels=1,rate=48000,layout=interleaved ! audioconvert ! pulsesink

 
 ## Developer Notes
 gst_agorasink_chain(...) in gstagorasink.c  is the main logic and entrypoint    
 meson.build specifies the files to be built    
 agorac.cpp is related to RTMPG project which we use here as a .so library  
 
