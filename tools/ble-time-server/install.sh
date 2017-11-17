#!/bin/bash

set -e
set -u

OS=`uname`

if [ "$OS" = "Darwin" ]; then
    which brew || /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    which node || brew install node
elif [ "$OS" = "Linux" ]; then
    which node || curl -sL https://deb.nodesource.com/setup_8.x | sudo -E bash -
    which node || sudo apt-get install -y nodejs
    which setcap || sudo apt-get install libcap2-bin
    sudo setcap cap_net_raw+eip $(eval readlink -f `which node`)
    sudo apt-get install libusb-1.0-0-dev
    sudo apt-get install bluetooth bluez libbluetooth-dev libudev-dev
fi

npm install
