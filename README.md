products-cozmo
==================

The Cozmo core engine.

******************************************************************************
NOTE: These build instructions are outdated, see the readme in anki/cozmo-game
******************************************************************************

==================


==========================
Building products-cozmo
==========================


-------------
XCode (Mac)
--------------

1) Build coretech external (see its local readme)

2) In products-cozmo/build --- create 'build' folder if it doesn't exist --- run
 
  cmake .. -G Xcode

3) Open Cozmo.xcodeproj/ and build


-------------
Microsoft Visual Studio 2012 MSVC (Windows)
-------------
1) Build coretech external (see its local readme)

2) In products-cozmo/build --- create 'build' folder if it doesn't exist --- run
 
  cmake -G"Visual Studio 11" ..

3) Open Cozmo.sln in Visual Studio 2012 and build


-------------
GCC on Ubuntu (Including Amazon EC2)
-------------

1) Install compilation tools
sudo apt-get install cmake
sudo apt-get install g++
sudo apt-get install git

2) Build coretech external (see its local readme)

3) In products-cozmo/build --- create 'build' folder if it doesn't exist --- run
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DEMBEDDED_USE_GTEST=0

4) Make the project (-j16 is for a 16 core machine)
make -j16

-------------
GCC on Cubietruck (Cortex-A7)
-------------

1) Install Ubuntu
1a) Download the image of Lubuntu 13.04: http://dl.cubieboard.org/software/a20-cubietruck/lubuntu/ct-lubuntu-nand-v1.03/EN/lubuntu-desktop-nand.img.gz
1b) Flash to NAND by using the instructions (with newer image file) at: http://docs.cubieboard.org/tutorials/ct1/installation/cb3_lubuntu-12.10-desktop_nand_installation_v1.00

2) Change the first line in the /etc/apt/sources.list to "deb http://old-releases.ubuntu.com/ubuntu/ quantal main universe"

3) Change your timezone, and verify that it worked by running "date"
sudo mv /etc/localtime /etc/localtime-old
sudo ln -sf /usr/share/zoneinfo/US/Pacific-New /etc/localtime
date

4) Mount the external hard drive with "sudo mount -o umask=000 /dev/sda4 /mnt/fastExtern

5) Install compilation tools
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install cmake g++ zlib1g-dev build-essential gcc-multilib

6) In products-cozmo/build --- create 'build' folder if it doesn't exist --- run cmake twice
cmake .. -DCMAKE_BUILD_TYPE=Release -DEMBEDDED_USE_GTEST=0 -DEMBEDDED_USE_MATLAB=0 -DEMBEDDED_USE_OPENCV=0
cmake .. -DCMAKE_BUILD_TYPE=Release -DEMBEDDED_USE_GTEST=0 -DEMBEDDED_USE_MATLAB=0 -DEMBEDDED_USE_OPENCV=0

7) Make the project
make -j3 ; python ../python/addSourceToGccAssembly.py /mnt/fastExtern/products-cozmo/build

-------------
GCC on Linux, cross-compiled for Cubietruck (Cortex-A7)
-------------

1) Follow steps 1-3 and 5 for "GCC on Cubietruck (Cortex-A7)"

2) Install cross-compilation on PC-based Ubuntu 12.04 or 14.04 (or probably others, though I haven't tested)
# On the PC, enter the following:
sudo add-apt-repository 'deb http://us.archive.ubuntu.com/ubuntu utopic main'
sudo apt-get update
sudo apt-get install cmake gcc-4.9-arm-linux-gnueabihf g++-4.9-arm-linux-gnueabihf
sudo add-apt-repository --remove 'deb http://us.archive.ubuntu.com/ubuntu utopic main'
sudo apt-get update

3) Build cross-compiled.  In products-cozmo/build --- create 'build' folder if it doesn't exist --- run cmake
AR=arm-linux-gnueabihf-gcc-ar-4.9 AS=arm-linux-gnueabihf-gcc-as-4.9 CC=arm-linux-gnueabihf-gcc-4.9 CXX=arm-linux-gnueabihf-g++-4.9 cmake .. -DCMAKE_BUILD_TYPE=Release -DEMBEDDED_USE_GTEST=0 -DEMBEDDED_USE_MATLAB=0 -DEMBEDDED_USE_OPENCV=0
make -j8

4) On the board, run sudo ifconfig to determine the inet addr (In the format as 192.168.1.???)

5) Set up ssh key
  # On the PC
  ssh-keygen -t rsa
  # For the file to save the key, choose something like "/home/yourUserId/.ssh/id_rsa_cubie"
  ssh linaro@192.168.1.125 mkdir -p .ssh
  cat .ssh/id_rsa_cubie.pub | ssh linaro@192.168.1.125 'cat >> .ssh/authorized_keys'

6) Comple, upload, and run on the board, by running the file "products-cozmo/runA7.sh"

