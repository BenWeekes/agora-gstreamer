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

 appid -- sets agora app id or token
 
 channel  -- sets agora channel id

 userid   -- sets agora userid to connect with (optional)
 
 remoteuserid -- specifies a single userid to subscribe to (optional)

 audio -- boolean (true/false) to specify if pipeline is audio     
 
 verbose -- boolean (true/false) to include logging output 
 

## Run and test

   export GST_PLUGIN_PATH=/usr/local/lib/x86_64-linux-gnu/gstreamer-1.0   
   
 ## agoraioudp

<ins>Video in/out from webcam</ins>     
 
gst-launch-1.0 -e v4l2src ! image/jpeg,width=640,height=360 ! jpegdec ! queue ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! queue ! agoraioudp appid=xxx channel=xxx outport=7372 inport=7373 verbose=false  ! queue ! decodebin ! queue ! glimagesink

<ins>Audio out of Agora to speaker </ins>     

gst-launch-1.0 -v udpsrc port=7372 ! audio/x-raw,format=S16LE,channels=1,rate=48000,layout=interleaved ! audioconvert ! pulsesink

<ins>Audio in from mic</ins>     

gst-launch-1.0 -v pulsesrc ! audioconvert ! opusenc ! udpsink host=127.0.0.1 port=7373

<ins>Token example</ins>

gst-launch-1.0 -v videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1 ! videoconvert ! x264enc key-int-max=60 tune=zerolatency !  queue ! agoraioudp appid="006e24ca3eb5db7440ea673061316187b06IAB63A2UQqvEo8f1Ou8yGA2d4nYbefdEqP+/YTS0z+JJR0kQgrCBkyDDIgBEAvsBNXKGYQQAAQDFLoVhAgDFLoVhAwDffFLoVhBADFLoVh"  channel=ttt userid=1001 outport=7372 inport=7373 verbose=false ! fakesink sync=false

   
 ## agorasink
   
<ins>Video into Agora from test source:</ins>    

gst-launch-1.0 -v videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1   ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agorasink appid=xxx channel=test silent=1

<ins>Video into Agora from webcam source:</ins>     

 gst-launch-1.0 v4l2src ! jpegdec ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agorasink appid=xxx channel=test silent=1
 
<ins>Audio into Agora from test source:</ins>    

gst-launch-1.0 -v audiotestsrc wave=sine ! audioconvert ! opusenc ! agorasink audio=true appid=xxx channel=test silent=1

<ins>Audio into Agora from microphone</ins>    
gst-launch-1.0 -v pulsesrc ! audioconvert ! opusenc ! agorasink audio=true appid=xxx channel=xxx

 ## agorasrc

<ins>Video out of Agora:</ins>    
   agorasrc can be used to read encoded h264 from an agora channel, here is an example pipleline:     
   
   gst-launch-1.0 agorasrc verbose=false appid=xxx channel=xxx  ! decodebin ! glimagesink     

   gst-launch-1.0 agorasrc verbose=false  appid=xxx channel=xxx! decodebin ! autovideosink      

   where appid and channel is same as agorasink. 
   
 <ins>Audio out of Agora</ins>
 
   gst-launch-1.0 agorasrc audio=true verbose=false appid=xxx channel=gstreamer ! filesink location=test.raw     
   
   gst-launch-1.0 agorasrc audio=true verbose=false appid=xxx channel=xxx ! audio/x-raw,format=S16LE,channels=1,rate=48000,layout=interleaved ! audioconvert ! pulsesink

## Camera Debug on Linux
sudo apt-get install -y v4l-utils    
v4l2-ctl --list-formats    
v4l2-ctl     
v4l2-ctl --list-devices     

 
 ## Developer Notes
 gst_agorasink_chain(...) in gstagorasink.c  is the main logic and entrypoint    
 meson.build specifies the files to be built    
 agorac.cpp is related to RTMPG project which we use here as a .so library  
 Uses this SDK wget https://download.agora.io/sdk/release/Agora-RTC-x86_64-linux-gnu-v3.4.217.tgz     
 tar -xvzf Agora-RTC-x86_64-linux-gnu-v3.4.217.tgz   
 sudo apt install cmake    
 cd  agora_rtc_sdk/example    
 ./build-x86_64.sh    
 LD_LIBRARY_PATH=/home/ben/agora_rtc_sdk/agora_sdk    
 export LD_LIBRARY_PATH     
 test data https://drive.google.com/file/d/1m1PzTCiV1AKy_mVYZA5la9WQtZu-acKI/view?usp=sharing     
 ./sample_send_h264_dual_stream --token xxxx --channelId xxxx --HighVideoFile ~/test_data/test_multi_slice.h264 --LowVideoFile ~/test_data/test_multi_slice.h264   
