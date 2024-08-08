
#! /bin/sh

#we fix agora sdk to 3.8.2, but we can change that when needed
export AGORA_SDK_DIR='/home/ubuntu/agora-gstreamer/agora/sdk/arm3822'

#building libagorac
cwd=$(pwd)
cd agora/libagorac && rm -rf build||true && mkdir build && cd build && cmake .. && make && sudo make install
cd $cwd


#build and install the plugins
cd gst-agora && rm -rf build||true && mkdir build && cd build && cmake  .. && make && sudo make install
