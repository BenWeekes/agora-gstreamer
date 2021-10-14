#! /bin/sh

BUILD_DIR=$PWD

#build libagorac
echo "building and installing libagorac ..."
cd agora/libagorac/ && ./install.sh $BUILD_DIR/agora/sdk

echo "building and installing agora plugin ..."
cd $BUILD_DIR/gst-agora && meson build && ./install

sudo cp $BUILD_DIR/demo.h264 /usr/local/lib/x86_64-linux-gnu/gstreamer-1.0
