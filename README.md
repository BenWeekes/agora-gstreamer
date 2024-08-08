A gstreamer wrapper for Agora Linux SDK (sink and src)
## Server Choice
A t3.medium AWS instance with 30GB of disk is a good starting point.
Ubuntu 20.04 and 22.04 both work fine.     
Intel or arm are supported.



## Install gstreamer and dependencies
   sudo apt-get update     
   sudo apt-get --fix-broken --fix-missing install -y libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio   

## Install additional libraries:

   sudo apt-get install -y meson libswscale-dev x264 libx264-dev libopus-dev   
   sudo apt install -y build-essential git libpcre3 libpcre3-dev zlib1g zlib1g-dev libssl-dev unzip     
   sudo apt install -y libavcodec-dev libavformat-dev libavutil-dev nasm libavfilter-dev libopus-dev   
   
## Test gstreamer install
gst-launch-1.0 -v videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1 ! queue ! fakesink    
 
## Build and install agora gstreamer plugins
   After installing the libraries above on your Ubuntu system you can build the latest Intel version (or ARM if required)           
   Clone this repo using git clone       
   
   cd agora-gstreamer/build     
  ./build_all_4.2.30.sh   # build latest version  (or ARM if required)
  
  If no errors are printed the new agora gs plugins will be installed on the system ready for use


## Pipeline Configuration Properties

 appid -- sets agora app id or token
 
 channel  -- sets agora channel id

 userid   -- sets agora userid to connect with (optional)
 
 remoteuserid -- specifies a single userid to subscribe to (optional)

 audio -- boolean (true/false) to specify if pipeline is audio     
 
 verbose -- boolean (true/false) to include logging output 
 

## Run and test
You must always run the following export before using any of these plugins     

   export GST_PLUGIN_PATH=/usr/local/lib/x86_64-linux-gnu/gstreamer-1.0   
   

 ## agoraioudp

<ins>Video in/out from webcam</ins>     
 
gst-launch-1.0 -e v4l2src ! image/jpeg,width=640,height=360 ! jpegdec ! queue ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! queue ! agoraioudp appid=xxx channel=xxx outport=7372 inport=7373 verbose=false  ! queue ! decodebin ! queue ! glimagesink

<ins>Audio out of Agora to speaker </ins>     

gst-launch-1.0 -v udpsrc port=7372 ! audio/x-raw,format=S16LE,channels=1,rate=48000,layout=interleaved ! audioconvert ! pulsesink

<ins>Audio in from mic</ins>     

gst-launch-1.0 -v pulsesrc ! audioconvert ! opusenc ! udpsink host=127.0.0.1 port=7373

<ins>Audio in from mic multiple cast</ins>   

gst-launch-1.0 -v pulsesrc ! audioconvert ! opusenc ! udpsink host=224.1.1.1 port=7373 auto-multicast=true

<ins>Token example</ins>

gst-launch-1.0 -v videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1 ! videoconvert ! x264enc key-int-max=60 tune=zerolatency !  queue ! agoraioudp appid="006e24ca3eb5db7440ea673061316187b06IAB63A2UQqvEo8f1Ou8yGA2d4nYbefdEqP+/YTS0z+JJR0kQgrCBkyDDIgBEAvsBNXKGYQQAAQDFLoVhAgDFLoVhAwDffFLoVhBADFLoVh"  channel=ttt userid=1001 outport=7372 inport=7373 out-audio-delay=0 out-video-delay=70 verbose=false ! fakesink sync=false

<ins>Synchronization</ins>

The Agora SDK returns encoded audio and video in sync with one another. Your system may have a different 'decode and present' path duration for audio or video. You can adjust the delay on either using the out-audio-delay=0 and out-video-delay=70 params in the agoraioudp plugin. Units a microseconds.   

<ins>Firewall Proxy</ins>  
add proxy=true to the agoraioudp param list and the plugin will use the proxy service if the call can't connect after a default timeout of 10000 ms.   
For the proxy to work you need to whitelist the ip:ports for the relevant region(s) listed here: https://docs-preprod.agora.io/en/Video/cloud_proxy_na?platform=Android     

Additional optional params are:      
proxytimeout=10000 proxyips=128.1.77.34,128.1.78.146      
proxyips are the signalling ips to use.
test/test_proxy.c has test code for proxy. 
 
   
 ## agorasink
   
<ins>Video into Agora from test source:</ins>    

gst-launch-1.0 -v videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=320,height=180,framerate=60/1   ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agorasink appid=xxx channel=test 

<ins>Video into Agora from webcam source:</ins>     

 gst-launch-1.0 v4l2src ! jpegdec ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! agorasink appid=xxx channel=test 
 
<ins>Audio into Agora from test source:</ins>    

gst-launch-1.0 -v audiotestsrc wave=sine ! audioconvert ! opusenc ! agorasink audio=true appid=xxx channel=test 

<ins>Video into Agora with audio from udp port 7373</ins>   
agorasink appid=xxx channel=test inport=7373

<ins>Audio into Agora from microphone</ins>    
gst-launch-1.0 -v pulsesrc ! audioconvert ! opusenc ! agorasink audio=true appid=xxx channel=xxx

<ins>Audio into Agora from AAC file</ins>    
gst-launch-1.0 urisourcebin uri=https://filesamples.com/samples/audio/aac/sample3.aac   ! aacparse ! faad ! audioresample ! audioconvert ! opusenc bitrate=128000 ! queue ! agorasink audio=true appid=xxx channel=xxx enforce-audio-duration=true

<ins>Video into Agora from mp4 file</ins>    
gst-launch-1.0  urisourcebin uri=https://sa-utils.agora.io/media/v223.mp4  ! decodebin  ! x264enc key-int-max=60 tune=zerolatency ! queue ! agorasink appid=xxx channel=xxx

<ins>RTSP from camera on network</ins>    

gst-launch-1.0 rtspsrc location=rtsp://admin:eee@1.2.3.4:2036/Streaming/Channels/102 latency=0 buffer-mode=auto ! decodebin ! queue ! videoconvert ! x264enc key-int-max=60 tune=zerolatency  !  agorasink appid=xxx channel=xxx   

<ins>RTSP without h264 transcode (avc-to-annexb)</ins>       

gst-launch-1.0  rtspsrc location=rtsp://admin:rrr@1.2.21.50:6005/Streaming/Channels/102 latency=0 buffer-mode=auto ! rtph264depay ! h264parse ! video/x-h264, stream-format=avc, alignment=au ! agorasink appid=20b7c51ff4c644ab80cf5a4e646b0537 channel=test2 avc-to-annexb=true       

<ins>MP4 audio and video</ins>       
gst-launch-1.0 urisourcebin uri=https://chatcatchat.s3.amazonaws.com/fwtest/badsync.mp4 name=src \
  src. ! decodebin name=decoder \
  decoder. ! queue name=video_queue ! videoconvert ! x264enc key-int-max=60 tune=zerolatency ! queue ! agorasink appid=xxx channel=xxx inport=7373 \
  decoder. ! queue name=audio_queue ! audioresample ! audioconvert ! opusenc bitrate=128000 ! udpsink host=127.0.0.1 port=7373


 ## agorasrc

<ins>Video out of Agora:</ins>    
   agorasrc can be used to read encoded h264 from an agora channel, here is an example pipleline:     
   
   gst-launch-1.0 agorasrc verbose=false appid=xxx channel=xxx ! decodebin ! glimagesink     

   gst-launch-1.0 agorasrc verbose=false appid=xxx channel=xxx ! decodebin ! autovideosink      

   gst-launch-1.0 agorasrc appid=xxx channel=xxx ! decodebin ! videoconvert ! jpegenc ! multifilesink location=%05d.jpg

   where appid and channel is same as agorasink. 
   
 <ins>Audio out of Agora</ins>
 
   gst-launch-1.0 agorasrc audio=true verbose=false appid=xxx channel=gstreamer ! filesink location=test.raw     
   
   gst-launch-1.0 agorasrc audio=true verbose=false appid=xxx channel=xxx ! audio/x-raw,format=S16LE,channels=1,rate=48000,layout=interleaved ! audioconvert ! pulsesink

## How to run the test programs:

$ cd agora-gstreamer/test

To compile all test programs:

$ ./c

To run any test just type its name:

$ ./endtest2


## Camera Debug on Linux
sudo apt-get install -y v4l-utils    
v4l2-ctl --list-formats-ext --device=/dev/video1    
v4l2-ctl     
v4l2-ctl --list-devices 

gst-launch-1.0 -v videotestsrc pattern=ball is-live=true ! video/x-raw,format=I420,width=1280,height=720,framerate=60/1 ! videoconvert ! x264enc key-int-max=60 tune=zerolatency !  queue ! decodebin ! queue ! autovideosink


 
 ## Developer Notes
 https://docs.agora.io/cn/RTSA/downloads?platform=Linux
 
 gst_agorasink_chain(...) in gstagorasink.c  is the main logic and entrypoint    
 meson.build specifies the files to be built    
 agorac.cpp is related to RTMPG project which we use here as a .so library  
 Uses this SDK wget https://download.agora.io/sdk/release/Agora-RTC-x86_64-linux-gnu-v3.4.217.tgz     
 tar -xvzf Agora-RTC-x86_64-linux-gnu-v3.4.217.tgz   
 sudo apt install cmake    
 sudo apt-get update    
 sudo apt-get install -y build-essential     
 cd  agora_rtc_sdk/example        
 ./build-x86_64.sh    
 LD_LIBRARY_PATH=/home/ben/agora_rtc_sdk/agora_sdk    
 export LD_LIBRARY_PATH     
 test data https://drive.google.com/file/d/1m1PzTCiV1AKy_mVYZA5la9WQtZu-acKI/view?usp=sharing     
./sample_send_h264_dual_stream --token xxxx --channelId iii --HighVideoFile ~/pro/agora_rtc_sdk/example/test_data/send_video.h264 --LowVideoFile ~/pro/agora_rtc_sdk/example/test_data/send_video.h264

 20b7c51ff4c644ab80cf5a4e646b05377

SDK Log ~/.agora/agorasdk.log

Jetson: Linux kernel architecture is aarch64 / arm64 (64-bit) (?)
PiL: gnueabihf (?)


## Creating and installing a binary release:

To create a binary release:

cd release
./make-release

To install the release on the target machine:

cd release
./install


## Cross compilation of Arm (Target) on x86 (Host)

(1) install gcc and G++ for arm 

sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

(2) make sure you set the environment variable INSTALL_PATH where the library will be copied to:

for example:

export INSTALL_PATH=/home/ubuntu/arm-dist

(3) copy  the dir /usr/include/aarch64-linux-gnu from the target to the host

this will allow g++-aarch64-linux-gnu to find the required libraries on the host

(4) compile libagorac by specifying arm config in cmake:

mkdir build && cd build && cmake -DCMAKE_TOOLCHAIN_FILE=arm64.cmake .. && make && sudo make install

(5) copy the installation files from host to target and test there


