#!/usr/bin/env python

# Usage: make_iOS_libs.py [build directory]    # Default build directory is build_ios/

import glob, re, os, os.path, shutil, string, sys

dstRoot = "build_ios"
config = "Debug"  

scriptdir = os.path.dirname(os.path.realpath(__file__))

# Build cozmo-engine libs first
#os.chdir("lib/anki/cozmo-engine")
#os.system("./make_iOS_libs.py")
#os.chdir(scriptdir)

#sys.exit()

# Get build directory if passed in
if len(sys.argv) > 1:
  dstRoot = str(sys.argv[1])

# Get directory of this script
currdir = os.getcwd()
if not os.path.isdir(dstRoot):
  os.makedirs(dstRoot)
os.chdir(dstRoot)


os.system("cmake -GXcode -DCMAKE_BUILD_TYPE=" + config + " -DANKI_IOS_BUILD=1 " + scriptdir)
    
print "Building all the libraries for each architecture"

# TODO: How to build these simultaneously?
# TODO: Programmatically loop over VALID_ARCHS

# iPhoneOS builds
validArchs = "armv7 arm64"
os.system("xcodebuild ARCHS=\"" + validArchs + "\"  -project CozmoGame_iOS.xcodeproj/ -jobs 8 -sdk iphoneos -configuration " + config + " -target ALL_BUILD")

# iPhoneSimulator builds
validArchs = "i386 x86_64"
os.system("xcodebuild ARCHS=\"" + validArchs + "\"  -project CozmoGame_iOS.xcodeproj/ -jobs 8 -sdk iphonesimulator -configuration " + config + " -target ALL_BUILD")

os.chdir(currdir)

# Join the all the architectures for each module into one library for each module
multiArchDir = dstRoot + "/multiArchLibs"
if os.path.isdir(multiArchDir):
  shutil.rmtree(multiArchDir)
os.makedirs(multiArchDir)

##
## Cozmo_Game
##
print "Building universal fat lib of Cozmo_Game"


targets = ["iPhoneOS", "iPhoneOS", "iPhoneSimulator", "iPhoneSimulator"]
archs   = ["armv7",    "arm64",    "i386",            "x86_64"         ]         


inputLibsList = []
for iArch in range(len(archs)):
  inputLibsList.append(dstRoot + "/src/CozmoGame_iOS.build/" + config + "-" + targets[iArch].lower() + 
                      "/Cozmo_Game.build/Objects-normal/" + archs[iArch] + "/libCozmo_Game.a")
                                             
os.system("lipo -create " + " ".join(inputLibsList) + " -o " + multiArchDir + "/libCozmo_Game.a")

os.chdir(scriptdir)

# Copy cozmo-engine libs to the cozmo-game ios build folder
cozmoEngineLibsDir = "lib/anki/cozmo-engine/build_ios/multiArchLibs"
cozmoEngineLibFiles = os.listdir(cozmoEngineLibsDir)
for libFileName in cozmoEngineLibFiles:
  fullLibFileName = os.path.join(cozmoEngineLibsDir, libFileName)
  shutil.copy(fullLibFileName, multiArchDir)


print "Done."


  
