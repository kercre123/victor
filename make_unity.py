#!/usr/bin/env python

# Usage: make_iOS_libs.py [build directory]    # Default build directory is build_ios/

import glob, re, os, os.path, shutil, string, sys

dstRoot = "build_unity"
config = "Debug"  

scriptdir = os.path.dirname(os.path.realpath(__file__))
unityLoc = os.path.join(scriptdir, "unity/Cozmo/Assets/Plugins/iOS/")

# Build cozmo-engine libs first
os.chdir("lib/anki/cozmo-engine")
os.system("./make_iOS_libs.py")
os.chdir(scriptdir)


# Get build directory if passed in
if len(sys.argv) > 1:
  dstRoot = str(sys.argv[1])

# Get directory of this script
currdir = os.getcwd()
if not os.path.isdir(dstRoot):
  os.makedirs(dstRoot)
os.chdir(dstRoot)


os.system("cmake -GXcode -DCMAKE_BUILD_TYPE=" + config + " -DANKI_IOS_BUILD=1 " + scriptdir)

#command = "xcodebuild -project CozmoGame_iOS.xcodeproj/ -jobs 8 -sdk iphoneos -configuration " + config + " -target ALL_BUILD clean"
#print command
#os.system(command)

print "Building all the libraries for each architecture"

# TODO: How to build these simultaneously?
# TODO: Programmatically loop over VALID_ARCHS

# iPhoneOS builds
validArchs = "armv7 arm64"
command = "xcodebuild ARCHS=\"" + validArchs + "\"  -project CozmoGame_iOS.xcodeproj/ -jobs 8 -sdk iphoneos -configuration " + config + " -target ALL_BUILD"
print command
os.system(command)

# iPhoneSimulator builds
validArchs = "i386 x86_64"
command = "xcodebuild ARCHS=\"" + validArchs + "\"  -project CozmoGame_iOS.xcodeproj/ -jobs 8 -sdk iphonesimulator -configuration " + config + " -target ALL_BUILD"
print(command)
os.system(command)

os.chdir(currdir)

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
gameLibsList = []
for iArch in range(len(archs)):
  inputLibsList.append(dstRoot + "/unity/CSharpBinding/CozmoGame_iOS.build/" + config + "-" + targets[iArch].lower() +
                       "/CSharpBinding.build/Objects-normal/" + archs[iArch] + "/libCSharpBinding.a")
  gameLibsList.append(dstRoot + "/src/CozmoGame_iOS.build/" + config + "-" + targets[iArch].lower() +
                       "/Cozmo_Game.build/Objects-normal/" + archs[iArch] + "/libCozmo_Game.a")

command = "lipo -create " + " ".join(inputLibsList) + " -o " + multiArchDir + "/libCSharpBinding.a"
print(command)
os.system(command)

command = "lipo -create " + " ".join(gameLibsList) + " -o " + multiArchDir + "/libCozmo_Game.a"
print(command)
os.system(command)

try:
    os.makedirs(unityLoc)
except:
    pass

shutil.copy(multiArchDir + "/libCSharpBinding.a", unityLoc)
shutil.copy(multiArchDir + "/libCozmo_Game.a", unityLoc)

os.chdir(scriptdir)

print "Copying libraries to unity."

# Copy cozmo-engine libs to the cozmo-game ios build folder
cozmoEngineLibsDir = "lib/anki/cozmo-engine/build_ios/multiArchLibs"
cozmoEngineLibFiles = os.listdir(cozmoEngineLibsDir)
for libFileName in cozmoEngineLibFiles:
  fullLibFileName = os.path.join(cozmoEngineLibsDir, libFileName)
  shutil.copy(fullLibFileName, unityLoc)

opencvLibsDir = os.path.join(os.environ.get("CORETECH_EXTERNAL_DIR"), "build/opencv-ios/multiArchLibs/")
opencvLibFiles = os.listdir(opencvLibsDir)
for libFileName in opencvLibFiles:
    fullLibFileName = os.path.join(opencvLibsDir, libFileName)
    shutil.copy(fullLibFileName, unityLoc)

print "Done."


  
