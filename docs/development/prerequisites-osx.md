# Prerequisites for building on OS X

Build prerequisites for OS X should be installed automatically as part of the build process.
This is currently the only part of the `configure.py` inherited from cozmo-one that is still required.

## Mac OS X

We currently support building on Mac OS 10.12. Older versions may also work, 
but we do not actively support them. If you run into problems trying to build 
on old versions of MacOS, your options are to figure it out or upgrade.

## Mac build environment

We use the clang compiler toolchain bundled with Xcode in order to build mac
binaries for tests and webots simulations. Production builds are currently
using Xcode 9.1.

You can install Xcode from the Mac App Store. Make sure you open XCode at least
once after installing / updating because it may ask you to accept terms of
service before it can be run from build scripts.

## Homebrew

If you don't already have [brew](http://brew.sh), get it now:

```
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

Then use brew to install the following dependencies. Note that Apple's included 
Python can cause problems, so it's recommended to install through brew:

```
brew install python3
brew install python
```
    
You will also need to install the python yaml module, `pyyaml`, for both python2 
and python3:

```
pip2 install pyyaml
pip3 install pyyaml
```

## Cleanup stale homebrew installations

Since the Android SDK and NDK are automatically downloaded, you should be using
`adb` from there. Make sure you do not have android-sdk installed from `brew` by
running:

```
brew uninstall android-sdk
brew uninstall android-ndk
```

## Android App build environment

The robotics and engine code run on an embedded android system and require both 
the Android NDK toolchain and SDK tools in order to build and deploy to devices. 
The correct versions will be automatically installed for you when you run 
`./configure.py` for the Android platform.

You can also install them ahead of time with:

```
./tools/build/tools/ankibuild/android.py --install-sdk
./tools/build/tools/ankibuild/android.py --install-ndk
```

Once the tools are installed, you can setup your shell environment for Android 
development by running the command:

```
source project/victor/envsetup.sh
```

