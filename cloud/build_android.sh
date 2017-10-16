#!/bin/bash
set -ex

export GOPATH="$PWD"
GIT=`which git`
TOPLEVEL=`$GIT rev-parse --show-toplevel`
: ${ANDROID_API:=24}
: ${ANDROID_NDK_HOME:=`$TOPLEVEL/tools/build/tools/ankibuild/android.py`}

BUILDROOT="$TOPLEVEL/_build/android/go"
mkdir -p $BUILDROOT

# Create native android toolchain
TOOLCHAINDIR="$BUILDROOT/toolchain"
if [ ! -d $TOOLCHAINDIR ]; then
	$ANDROID_NDK_HOME/build/tools/make_standalone_toolchain.py --install-dir=$TOOLCHAINDIR --arch=arm --api=$ANDROID_API --stl=libc++
	# The command above includes the wrong headers due to a bug in the android sdk and/or cmake. The following two commands fix this issue.
	rm -rf $TOOLCHAINDIR/sysroot/usr
	cp -r "$ANDROID_NDK_HOME/platforms/android-$ANDROID_API/arch-arm/usr" "$TOOLCHAINDIR/sysroot/usr"
fi

# Build go process
OUTDIR="$BUILDROOT/out"
mkdir -p $OUTDIR
GOOS=android GOARCH=arm GOARM=7 go get -d
CC="$TOOLCHAINDIR/bin/arm-linux-androideabi-gcc" \
   CXX="$TOOLCHAINDIR/bin/arm-linux-androideabi-g++" \
   CGO_ENABLED=1 CGO_FLAGS="-march=armv7-a" \
   GOOS=android GOARCH=arm GOARM=7 \
   go build -i -o $OUTDIR/cloudmain -pkgdir $BUILDROOT/gopkg
