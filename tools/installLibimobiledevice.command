#!/bin/bash
sudo pwd

mkdir ~/libimobiledeviceinstall
cd ~/libimobiledeviceinstall

git clone https://github.com/libimobiledevice/libplist
git clone https://github.com/libimobiledevice/libusbmuxd
git clone https://github.com/libimobiledevice/libimobiledevice

brew install openssl

cd libplist
./autogen.sh
make
sudo make install
cd ~/libimobiledeviceinstall

cd libusbmuxd
./autogen.sh
make
sudo make install
cd ~/libimobiledeviceinstall

cd libimobiledevice
./autogen.sh PKG_CONFIG_PATH=/usr/local/opt/openssl/lib/pkgconfig
make
sudo make install
cd ~/libimobiledeviceinstall
