#! /bin/sh

# requirements before running this script:
#(1) installed required compilers and libraries on host: sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
#(2) also install all required gestreamer libs on the target
#(3) make sure you have copied include/aarch64-linux-gnu  from target (/usr/local/include) to host (/usr/include/)
#    so at the host you should have the dir  /usr/include/aarch64-linux-gnu  
#(4) make sure you have copied lib/aarch64-linux-gnu  from target (/usr/local/lib) to host (/usr/lib/)
#    so at the host you should have the dir  /usr/lib/aarch64-linux-gnu 
#(5) step #3 and #3 should be done after installing all required libs and gestreamer on target (step 3)

#this is where we collect the binaries
INSTALL_DIR='/home/ubuntu/arm-dist'
export INSTALL_PATH=$INSTALL_DIR

#better to recreate the install dir as it might not been created before
rm -rf $INSTALL_DIR || true

mkdir -p $INSTALL_DIR/lib
mkdir -p $INSTALL_DIR/include

#we fix agora sdk to 3.8.2, but we can change that when needed
export AGORA_SDK_DIR="$PWD/../agora/sdk/agora_sdk_aarch64_4.2.32"

#building libagorac
cwd=$(pwd)
cd ../agora/libagorac && rm -rf build||true && mkdir build && cd build && cmake -DCMAKE_TOOLCHAIN_FILE=arm64.cmake .. && make && sudo make install
cd $cwd

#we need to copy libagorac header to /usr/include/aarch64-linux-gnu so that the compiler
#can find them when it build the plugins

cp $INSTALL_DIR/include/*.* /usr/include/aarch64-linux-gnu

#build and install the plugins
cd gst-agora && rm -rf build||true && mkdir build && cd build && cmake -DCMAKE_TOOLCHAIN_FILE=arm64.cmake .. && make && sudo make install

