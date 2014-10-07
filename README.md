products-cozmo
==================

The main Cozmo repository

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
sudo apt-get install cmake g++ zlib1g-dev build-essential

6) In products-cozmo/build --- create 'build' folder if it doesn't exist --- run cmake twice
cmake .. -DCMAKE_BUILD_TYPE=Release -DEMBEDDED_USE_GTEST=0 -DEMBEDDED_USE_MATLAB=0 -DEMBEDDED_USE_OPENCV=0
cmake .. -DCMAKE_BUILD_TYPE=Release -DEMBEDDED_USE_GTEST=0 -DEMBEDDED_USE_MATLAB=0 -DEMBEDDED_USE_OPENCV=0

7) Make the project
make -j3