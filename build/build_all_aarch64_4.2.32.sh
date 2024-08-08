#! /bin/sh

BUILD_DIR=$PWD

#build libagorac
echo "building and installing libagorac ..."
cd ../agora/libagorac/ && ./install.sh $BUILD_DIR/../agora/sdk/agora_sdk_aarch64_4.2.32

echo "building and installing agora plugin ..."
sudo rm -rf $BUILD_DIR/../gst-agora/build || true
cd $BUILD_DIR/../gst-agora && meson build && ./install

