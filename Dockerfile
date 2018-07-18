FROM node:8-stretch

RUN apt-get update && apt-get install -y \
    android-tools-fsutils \
    bison \
    build-essential \
    chrpath \
    curl \
    dos2unix \
    flex \
    g++-multilib \
    gawk \
    gcc-multilib \
    genisoimage \
    git-core \
    gnupg \
    gperf \
    lib32ncurses5-dev \
    lib32z-dev \
    libc6-dev \
    libc6-dev-i386 \
    libgl1-mesa-dev \
    libsndfile-dev \
    libssl-dev \
    libx11-dev \
    libxml-simple-perl \
    libxml2-utils \
    nodejs \
    p7zip-full \
    python-markdown \
    ruby \
    subversion \
    texinfo \
    tofrodos \
    wget \
    x11proto-core-dev \
    xsltproc \
    zip \
    zlib1g-dev

RUN chmod 0755 /usr/local/bin
RUN wget https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-linux.zip
RUN unzip ninja-linux.zip -d /usr/local/bin/
RUN update-alternatives --install /usr/bin/ninja ninja /usr/local/bin/ninja 1 --force
RUN ln -sf /usr/bin/nodejs /usr/local/bin/node
