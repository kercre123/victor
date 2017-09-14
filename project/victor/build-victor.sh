#!/bin/bash

set -e

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
_SCRIPT_NAME=`basename ${0}`
TOPLEVEL=$(cd "${SCRIPT_PATH}/../.." && pwd)
BUILD_TOOLS="${TOPLEVEL}/tools/build/tools"

function usage() {
    echo "$_SCRIPT_NAME [OPTIONS]"
    echo "  -h                      print this message"
    echo "  -v                      print verbose output"
    echo "  -c [CONFIGURATION]      build configuration {Debug,Release}"
    echo "  -p [PLATFORM]           build target platform {android,mac}"
    echo "  -g [CMAKE_GENERATOR]    CMake generator {Ninja,Xcode}"
    echo "  -f                      force-run filelist updates and cmake configure before building"
    echo "  -d                      DEBUG: generate file lists and exit"
    echo "  -x [CMAKE_EXE]          path to cmake executable"
    echo "  -C                      generate build config and exit without building"
}

#
# defaults
#
VERBOSE=0
CONFIGURE=0
GEN_SRC_ONLY=0
RUN_BUILD=1
CMAKE_EXE="${HOME}/.anki/cmake/dist/3.8.1/CMake.app/Contents/bin/cmake"

CONFIGURATION=Debug
PLATFORM=android
CMAKE_GENERATOR=Ninja


while getopts ":x:c:p:g:hvfdC" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        v)
            VERBOSE=1
            ;;
        f)
            CONFIGURE=1
            ;;
        C)
            CONFIGURE=1
            RUN_BUILD=0
            ;;
        d)
            CONFIGURE=1
            GEN_SRC_ONLY=1
            ;;
        x)
            CMAKE_EXE="${OPTARG}"
            ;;
        b)
            BUILD_DIR="${OPTARG}"
            ;;
        c)
            CONFIGURATION="${OPTARG}"
            ;; 
        p)
            PLATFORM="${OPTARG}"
            ;;
        g)
            CMAKE_GENERATOR="${OPTARG}"
            ;;
        :)
            echo "Option -${OPTARG} required an argument." >&2
            usage
            exit 1
            ;;
    esac
done

# Move past getops args
shift $(($OPTIND - 1))

#
# settings
#


if [ ! -d "${TOPLEVEL}/generated" ] || [ ! -d "${TOPLEVEL}/EXTERNALS" ]; then
    echo "Missing ${TOPLEVEL}/generated or ${TOPLEVEL}/EXTERNALS"
    echo "Attempting to run configure.py" 
    pushd robot > /dev/null 2>&1
    make dev2
    popd > /dev/null 2>&1
    ${TOPLEVEL}/configure.py -2 -p ${PLATFORM}  generate
fi

PLATFORM=`echo $PLATFORM | tr "[:upper:]" "[:lower:]"`

# For non-ninja builds, add generator type fo build dir
BUILD_SYSTEM_TAG=""
if [ ${CMAKE_GENERATOR} != "Ninja" ]; then
    BUILD_SYSTEM_TAG="-${CMAKE_GENERATOR}"
fi
: ${BUILD_DIR:="${TOPLEVEL}/_build/${PLATFORM}/${CONFIGURATION}${BUILD_SYSTEM_TAG}"}

#declare -A PROJECT_MAP
PROJECT_MAP["Ninja"]="build.ninja"
PROJECT_MAP["Xcode"]="cozmo.xcodeproj"

if [ ${PROJECT_MAP[${CMAKE_GENERATOR}]+_} ]; then
    # found
    if [ ! -e "${BUILD_DIR}/${PROJECT_MAP[${CMAKE_GENERATOR}]}" ]; then
        CONFIGURE=1
    fi
else
    # not found
    echo "Unsupported CMake generator: ${CMAKE_GENERATOR}"
    exit 1
fi

: ${CMAKE_MODULE_DIR:="${TOPLEVEL}/cmake"}

if [ ! -f ${CMAKE_EXE} ]; then
  echo "CMake executable not found. For use with Android, install CMake with Android SDK"
  exit 1
fi

#
# generate source file lists
#

if [ $CONFIGURE -eq 1 ]; then
    mkdir -p ${BUILD_DIR}

    GEN_SRC_DIR="${TOPLEVEL}/generated/cmake"
    mkdir -p "${GEN_SRC_DIR}"

    if [ $VERBOSE -eq 1 ]; then
        METABUILD_VERBOSE="-v"
    else
        METABUILD_VERBOSE=""
    fi
    ${BUILD_TOOLS}/metabuild/metabuild.py $METABUILD_VERBOSE -o ${GEN_SRC_DIR} \
        androidHAL/BUILD.in \
        animProcess/BUILD.in \
        clad/BUILD.in \
        engine/BUILD.in \
        coretech/common/BUILD.in \
        coretech/common/clad/BUILD.in \
        coretech/vision/BUILD.in \
        coretech/vision/clad/BUILD.in \
        coretech/planning/BUILD.in \
        coretech/messaging/BUILD.in \
        engine/BUILD.in \
        lib/util/source/anki/util/BUILD.in \
        resources/BUILD.in \
        robot/BUILD.in \
        robot/clad/BUILD.in \
        robot2/BUILD.in \
        simulator/BUILD.in \
        test/BUILD.in

    if [ $GEN_SRC_ONLY -eq 1 ]; then
        exit 0
    fi
fi

pushd ${BUILD_DIR} > /dev/null 2>&1

if [ $CONFIGURE -eq 1 ]; then

    if [ $VERBOSE -eq 1 ]; then
        VERBOSE_ARG="-DCMAKE_VERBOSE_MAKEFILE=ON"
    else
        VERBOSE_ARG=""
    fi

    PLATFORM_ARGS=""
    if [ "$PLATFORM" == "mac" ]; then
        PLATFORM_ARGS=(
            -DMACOSX=1
            -DANDROID=0
        )
    elif [ "$PLATFORM" == "android" ]; then
        PLATFORM_ARGS=(
            -DMACOSX=0
            -DANDROID=1
            -DANDROID_NDK=${HOME}/.anki/android/ndk-repository/android-ndk-r15b
            -DCMAKE_TOOLCHAIN_FILE="${CMAKE_MODULE_DIR}/android.toolchain.patched.cmake"
            -DANDROID_TOOLCHAIN_NAME=clang
            -DANDROID_ABI='armeabi-v7a with NEON'
            -DANDROID_NATIVE_API_LEVEL=24
            -DANDROID_PLATFORM=android-24
            -DANDROID_STL=c++_shared
            -DANDROID_CPP_FEATURES='rtti exceptions'
        )
    else
        echo "unknown platform: ${PLATFORM}"
        exit 1
    fi

    $CMAKE_EXE ${TOPLEVEL} \
        ${VERBOSE_ARG} \
        -G${CMAKE_GENERATOR} \
        -DCMAKE_BUILD_TYPE=${CONFIGURATION} \
        -DBUILD_SHARED_LIBS=1 \
        "${PLATFORM_ARGS[@]}"
        
fi

if [ $RUN_BUILD -ne 1 ]; then
    exit 0
fi

# Use shake (http://shakebuild.com/) to execute ninja files (if available)
# Shake provides a `--digest-and-input` option that uses digests of inputs instead
# of only file mtimes and therefore avoid unnecessary rebuilds caused by branch changes
set +e
which shake > /dev/null 2>&1
: ${USE_SHAKE:=$?}
set -e

if [ $USE_SHAKE -eq 0 ]; then
  VERBOSE_ARG=""
  if [ $VERBOSE -eq 1 ]; then
    VERBOSE_ARG="--verbose"
  fi
  shake --digest-and-input --report -j $VERBOSE_ARG $*
else
  $CMAKE_EXE --build . $*
fi

popd > /dev/null 2>&1
