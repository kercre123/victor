FROM ubuntu:bionic

RUN apt-get update && apt-get install -y \
    android-tools-fsutils \
    bison \
    build-essential \
    chrpath \
    curl \
    ccache \
    dos2unix \
    flex \
    gawk \
    genisoimage \
    git-core \
    gnupg \
    gperf \
    libncurses5-dev \
    libz-dev \
    libc6-dev \
    libcurl4-gnutls-dev \
    libgl1-mesa-dev \
    libsndfile-dev \
    libssl-dev \
    libx11-dev \
    libxml-simple-perl \
    libxml2-utils \
    p7zip-full \
    python-markdown \
    ruby \
    software-properties-common \
    subversion \
    texinfo \
    tofrodos \
    wget \
    uuid-dev \
    x11proto-core-dev \
    xsltproc \
    zip \
    cmake \
    autoconf \
    ninja-build \
    libtool \
    git-lfs \
    rsync \
    zlib1g-dev

#RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
#RUN apt-add-repository "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-7 main"
RUN apt-get update && apt-get install -y clang \
    libc++-dev 
#RUN ln -sf /usr/bin/clang++-7 /usr/bin/clang++

# RUN curl -sL https://deb.nodesource.com/setup_8.x | bash -
# RUN apt-get update && apt-get install -y nodejs

#RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | bash
#RUN apt-get install git-lfs

RUN chmod 0755 /usr/local/bin
#RUN wget https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-linux-aarch64.zip
#RUN unzip ninja-linux-aarch64.zip -d /usr/local/bin/
RUN ln -sf /usr/bin/ninja /usr/local/bin/ninja
#RUN update-alternatives --install /usr/bin/ninja ninja /usr/local/bin/ninja 1 --force
RUN ln -sf /usr/bin/nodejs /usr/local/bin/node
