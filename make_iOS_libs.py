#!/usr/bin/env python

# Usage: make_iOS_libs.py [build directory]    # Default build directory is build_ios/

import glob, re, os, os.path, shutil, string, sys

dstRoot = "build_ios"
config = "Debug"  

# Get build directory if passed in
if len(sys.argv) > 1:
  dstRoot = str(sys.argv[1])

# Get directory of this script
currdir = os.getcwd()
scriptdir = os.path.dirname(os.path.realpath(__file__))
if not os.path.isdir(dstRoot):
  os.makedirs(dstRoot)
os.chdir(dstRoot)

os.system("cmake -GXcode -DCMAKE_BUILD_TYPE=" + config + " -DANKI_IOS_BUILD=1 " + scriptdir)
    
print "Building all the libraries for each architecture"

# TODO: How to build these simultaneously?
# TODO: Programmatically loop over VALID_ARCHS

# iPhoneOS builds
validArchs = "armv7 arm64"
os.system("xcodebuild ARCHS=\"" + validArchs + "\" -jobs 8 -sdk iphoneos -configuration " + config + " -target ALL_BUILD")

# iPhoneSimulator builds
validArchs = "i386 x86_64"
os.system("xcodebuild ARCHS=\"" + validArchs + "\" -jobs 8 -sdk iphonesimulator -configuration " + config + " -target ALL_BUILD")

os.chdir(currdir)

# Join the all the architectures for each module into one library for each module
multiArchDir = dstRoot + "/multiArchLibs"
if os.path.isdir(multiArchDir):
  shutil.rmtree(multiArchDir)
os.makedirs(multiArchDir)

##
## Coretech_[MODULE]_[Robot/Basestation]
##
print "Building universal fat libs from all the coretech libs"

coretechNames = ["Common", "Common", "Vision", "Vision", "Messaging", "Planning"]
bsOrRobot     = ["Basestation", "Robot", "Basestation", "Robot", "Basestation", "Basestation"]
   
targets = ["iPhoneOS", "iPhoneOS", "iPhoneSimulator", "iPhoneSimulator"]
archs   = ["armv7",    "arm64",    "i386",            "x86_64"         ]         


for iLib in range(len(coretechNames)):
  libName = "CoreTech_" + coretechNames[iLib] + "_" + bsOrRobot[iLib]        
  inputLibsList = []
  for iArch in range(len(archs)):
    inputLibsList.append(dstRoot + "/coretech/" + coretechNames[iLib].lower() + "/" + 
                         bsOrRobot[iLib].lower() + "/src/CozmoEngine_iOS.build/" + config + "-" + targets[iArch].lower() + "/" +
                         libName + ".build/Objects-normal/" + archs[iArch] + "/lib" + libName + ".a")
  os.system("lipo -create " + " ".join(inputLibsList) + " -o " + multiArchDir + "/lib" + libName + ".a")

##
## Cozmo_Basestation
##
print "Building universal fat lib of Cozmo_Basestation"

inputLibsList = []
for iArch in range(len(archs)):
  inputLibsList.append(dstRoot + "/basestation/src/CozmoEngine_iOS.build/" + config + "-" + targets[iArch].lower() + 
                      "/Cozmo_Basestation.build/Objects-normal/" + archs[iArch] + "/libCozmo_Basestation.a")
                                             
os.system("lipo -create " + " ".join(inputLibsList) + " -o " + multiArchDir + "/libCozmo_Basestation.a")

##
## Cozmo_Game
##
print "Building universal fat lib of Cozmo_Game"

inputLibsList = []
for iArch in range(len(archs)):
  inputLibsList.append(dstRoot + "/game/src/CozmoEngine_iOS.build/" + config + "-" + targets[iArch].lower() + 
                      "/Cozmo_Game.build/Objects-normal/" + archs[iArch] + "/libCozmo_Game.a")
                                             
os.system("lipo -create " + " ".join(inputLibsList) + " -o " + multiArchDir + "/libCozmo_Game.a")

##
## jsoncpp
##
print "Building universal fat lib of jsoncpp"

inputLibsList = []
for iArch in range(len(archs)):
  inputLibsList.append(dstRoot + "/jsoncpp/CozmoEngine_iOS.build/" + config + "-" + targets[iArch].lower() + 
                      "/jsoncpp.build/Objects-normal/" + archs[iArch] + "/libjsoncpp.a")
                      
os.system("lipo -create " + " ".join(inputLibsList) + " -o " + multiArchDir + "/libjsoncpp.a")                    

print "Done."


  
