products-cozmo
==================

The main Cozmo repository

==================


==========================
Building products-cozmo
==========================


XCode (Mac)
--------------

1) Build coretech external (see its local readme)

2) In products-cozmo/build --- create 'build' folder if it doesn't exist --- run
 
  cmake .. -G Xcode

3) Open Cozmo.xcodeproj/ and build


Microsoft Visual Studio 2012 MSVC (Windows)
-------------
1) Build coretech external (see its local readme)

2) In products-cozmo/build --- create 'build' folder if it doesn't exist --- run
 
  cmake -G"Visual Studio 11" ..

3) Open Cozmo.sln in Visual Studio 2012 and build


GCC on Ubuntu (Including Amazon EC2)
-------------

1) Install compilation tools
sudo apt-get install cmake
sudo apt-get install g++
sudo apt-get install git

2) Build coretech external (see its local readme)

3) In products-cozmo/build --- create 'build' folder if it doesn't exist --- run
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DEMBEDDED_USE_GTEST=0

4) Make the files (-j16 is for a 16 core machine)
make -j16
