#!/usr/bin/env bash
set -eu

# Where is git?
GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi                                                                                                  
TOPLEVEL=`$GIT rev-parse --show-toplevel`

: ${ANKI_PROFILE_SYMBOLCACHE:="$1"}
: ${FORCE:=0}
: ${BUILD_ROOT:="${TOPLEVEL}/_build/android/Release"}
: ${INSTALL_ROOT:="/anki"}

# Warn if BUILD_ROOT points to Debug output
# Should only profile Release builds.
case $BUILD_ROOT in *Debug)
    echo "warning: You are attempting to profile a Debug build."
    echo "         Please use Release builds for profiling to ensure accurate measurements"

    # exit unless FORCE variable is set
    [ $FORCE -eq 0 ] && exit 1
esac

if [ ! -d ${BUILD_ROOT} ]; then
    echo "error: Build artifacts not found at ${BUILD_ROOT}"
    exit 1
fi

# Path to native executables on build host
native_bin_dir="${BUILD_ROOT}/bin"
native_lib_dir="${BUILD_ROOT}/lib"

# Path to native executables on device
device_bin_dir="${INSTALL_ROOT}/bin"
device_lib_dir="${INSTALL_ROOT}/lib"

# Path to symbolicated version of native executables
symbol_bin_dir="${ANKI_PROFILE_SYMBOLCACHE}${device_bin_dir}"
symbol_lib_dir="${ANKI_PROFILE_SYMBOLCACHE}${device_lib_dir}"

#
# Build a "shadow filesystem" of links that will be used for symbol lookups.
# Each link maps from a stripped file, as laid out on the device, to the
# corresponding unstripped file as laid out on the build host.
#

mkdir -p ${symbol_bin_dir}
mkdir -p ${symbol_lib_dir}

for full in ${native_bin_dir}/*.full ; do
  ln -sf ${full} ${symbol_bin_dir}/$(basename ${full%.full})
done

for full in ${native_lib_dir}/*.full ; do
  ln -sf ${full} ${symbol_lib_dir}/$(basename ${full%.full})
done
