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

# Path to native executables on build host
native_bin_dir="${TOPLEVEL}/_build/android/Debug/bin"
native_lib_dir="${TOPLEVEL}/_build/android/Debug/lib"

# Path to native executables on device
device_bin_dir="/data/data/com.anki.cozmoengine/bin"
device_lib_dir="/data/data/com.anki.cozmoengine/lib"

# Path to symbolicated version of native executables
symbol_bin_dir="symbol_cache/${device_bin_dir}"
symbol_lib_dir="symbol_cache/${device_lib_dir}"

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

