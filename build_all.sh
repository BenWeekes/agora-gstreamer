#! /bin/sh

BUILD_DIR=$PWD

#build libagorac
echo "building and installing libagorac ..."
cd agora/libagorac/ && ./install.sh $BUILD_DIR/agora/agora_sdk

echo "building and installing agora plugin ..."
cd $BUILD_DIR/gst-agora && meson build && ./install
