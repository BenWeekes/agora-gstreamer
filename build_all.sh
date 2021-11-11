#! /bin/sh

BUILD_DIR=$PWD

#build libagorac
echo "building and installing libagorac ..."
cd agora/libagorac/ && ./install.sh $BUILD_DIR/agora/sdk

sudo rm $BUILD_DIR/gst-agora/plugin-src/meson.build || true
cp $BUILD_DIR/gst-agora/plugin-src/gnu.build $BUILD_DIR/gst-agora/plugin-src/meson.build

echo "building and installing agora plugin ..."
sudo rm -rf $BUILD_DIR/gst-agora/build || true
cd $BUILD_DIR/gst-agora && meson build && ./install

