#!/usr/bin/env bash

if [ ! -h /usr/bin/clang ] && [ ! -h /usr/bin/clang++ ]; then
  echo ">>> Updating apt-get"
  sudo apt-get update --fix-missing

  echo ">>> Installing Valgrind v3.10.1"
  cd /opt/
  sudo apt-get install valgrind -y --fix-missing

  echo ">>> Installing Valgrind Dependencies"
  sudo apt-get -y install \
  curl \
  tree \
  build-essential \
  gyp \
  ninja-build \
  clang-3.6 \
  libc++-dev \
  mono-mcs \
  git \
  libboost1.55-dev \
  cmake \
  libreadline-dev \
  libglib2.0-dev \
  libuv-dev \
  dbus*-dev \
  libbluetooth-dev \
  python2.7 \
  python-pip \
  unzip

  sudo ln -s /usr/bin/clang-3.6 /usr/bin/clang
  sudo ln -s /usr/bin/clang++-3.6 /usr/bin/clang++

else
  echo ">>> Already privisioned Vagrant for Linux Valgrind run"
fi
