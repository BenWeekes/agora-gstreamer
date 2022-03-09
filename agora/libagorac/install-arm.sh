#! /bin/sh

INPUT_DIR=$1;
export AGORA_SDK_DIR=$INPUT_DIR

echo "MAKE"
make 
echo "MAKED"
sudo make install
sudo cp  $INPUT_DIR/agora_sdk/libagora_rtc_sdk.so  /usr/local/lib
sudo cp agorac.h /usr/local/include
sudo ldconfig
