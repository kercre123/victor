FROM ubuntu:16.04

ENV DEBIAN_FRONTEND=noninteractive

RUN \
    echo "------ basic setup ------" && \
    apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y --no-install-recommends \
        apt-utils \
        sudo \
        software-properties-common \
        curl \
        locales && \
    \
    echo "------ install a few ubuntu packages -------" && \
    apt-get update && \
    apt-get -y install \
      autoconf \
      bc \
      bison \
      build-essential \
      cmake \
      debianutils \
      diffstat \
      dos2unix \
      gawk \
      git \
      git-core \
      unzip \
      zip && \
    \
    echo "------ use bash for /bin/sh  -------" && \
    ln -sf /bin/bash /bin/sh && \
    echo "------ finishing up...  -------"



WORKDIR /build
COPY build-flatc-host-tools.sh /build/
VOLUME /build/out