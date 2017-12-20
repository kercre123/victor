#!/bin/bash

set -e

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=`basename ${0}`
TOPLEVEL=$(cd "${SCRIPT_PATH}/../.." && pwd)
BUILD_TOOLS="${TOPLEVEL}/tools/build/tools"

function usage() {
    echo "$SCRIPT_NAME [OPTIONS]"
    echo "  -h                      print this message"
    echo "  -v                      print verbose output"
    echo "  -c [CONFIGURATION]      build configuration {Debug,Release}"
    echo "  -p [PLATFORM]           build target platform {android,mac}"
    echo "  -g [CMAKE_GENERATOR]    CMake generator {Ninja,Xcode}"
    echo "  -f                      force-run filelist updates and cmake configure before building, and force-copy assets"
    echo "  -d                      DEBUG: generate file lists and exit"
    echo "  -x [CMAKE_EXE]          path to cmake executable"
    echo "  -C                      generate build config and exit without building"
    echo "  -F [FEATURE]            enable feature {factoryTest,factoryTestDev}"
    echo "  -T                      list all cmake targets"
    echo "  -t [target]             build specified cmake target"
    echo "  -e                      export compile commands"
}

#
# defaults
#
VERBOSE=0
CONFIGURE=0
GEN_SRC_ONLY=0
RM_BUILD_ASSETS=0
RUN_BUILD=1
CMAKE_TARGET=""
CMAKE_EXE="${HOME}/.anki/cmake/dist/3.8.1/CMake.app/Contents/bin/cmake"
EXPORT_COMPILE_COMMANDS=0

CONFIGURATION=Debug
PLATFORM=android
CMAKE_GENERATOR=Ninja
FEATURES=""

# TODO: provide command line option?
# NOTE: these can't both be true (but can both be false)
USE_TENSORFLOW=0
USE_TENSORFLOW_LITE=0

while getopts ":x:c:p:t:g:F:hvfdCTe" opt; do
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
            RM_BUILD_ASSETS=1
            ;;
        C)
            CONFIGURE=1
            RUN_BUILD=0
            ;;
        d)
            CONFIGURE=1
            GEN_SRC_ONLY=1
            ;;
        T)
            CMAKE_TARGET="help"
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
        F)
            FEATURES="${FEATURES} ${OPTARG}"
            ;;
        t)
            CMAKE_TARGET="${OPTARG}"
            ;;
        e)
            EXPORT_COMPILE_COMMANDS=1
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

echo "Attempting to run fetch-build-deps.sh"
${TOPLEVEL}/project/victor/scripts/fetch-build-deps.sh

PLATFORM=`echo $PLATFORM | tr "[:upper:]" "[:lower:]"`

#
# Validate configuration
#
case "${CONFIGURATION}" in
  [Dd][Ee][Bb][Uu][Gg])
    CONFIGURATION="Debug"
    ;;
  [Rr][Ee][Ll][Ee][Aa][Ss][Ee])
    CONFIGURATION="Release"
    ;;
  *)
    echo "${SCRIPT_NAME}: Unknown configuration '${CONFIGURATION}'"
    usage
    exit 1
    ;;
esac

#
# Enable feature flags
#
FEATURE_FLAGS=""

for feature in ${FEATURES} ; do
  case $feature in
    factoryTest)
      FEATURE_FLAGS="${FEATURE_FLAGS} -DFACTORY_TEST=1"
      ;;
    factoryTestDev)
      FEATURE_FLAGS="${FEATURE_FLAGS} -DFACTORY_TEST=1 -DFACTORY_TEST_DEV=1"
      ;;
    *)
      echo "${SCRIPT_NAME}: Unknown feature '${feature}'"
      usage
      exit 1
      ;;
  esac
done

#
# Enable export flags
#
EXPORT_FLAGS=""
if [ ${EXPORT_COMPILE_COMMANDS} -ne 0 ]; then
  EXPORT_FLAGS="-DCMAKE_EXPORT_COMPILE_COMMANDS=${EXPORT_COMPILE_COMMANDS}"
fi

# For non-ninja builds, add generator type to build dir
BUILD_SYSTEM_TAG=""
if [ ${CMAKE_GENERATOR} != "Ninja" ]; then
    BUILD_SYSTEM_TAG="-${CMAKE_GENERATOR}"
fi
: ${BUILD_DIR:="${TOPLEVEL}/_build/${PLATFORM}/${CONFIGURATION}${BUILD_SYSTEM_TAG}"}

case ${CMAKE_GENERATOR} in
    "Ninja")
        PROJECT_FILE="build.ninja"
        ;;
    "Xcode")
        PROJECT_FILE="cozmo.xcodeproj"
        ;;
    "*")
        PROJECT_FILE=""
        ;;
esac

if [ ${PROJECT_FILE}+_} ]; then
    # found
    if [ ! -e "${BUILD_DIR}/${PROJECT_FILE}" ]; then
        CONFIGURE=1
    fi
else
    # not found
    echo "Unsupported CMake generator: ${CMAKE_GENERATOR}"
    exit 1
fi

: ${CMAKE_MODULE_DIR:="${TOPLEVEL}/cmake"}

if [ ! -f ${CMAKE_EXE} ]; then
  echo "Missing CMake executable: ${CMAKE_EXE}"
  echo "Fetch the required CMake version by running ${TOPLEVEL}/tools/build/tools/ankibuild/cmake.py"
  echo "Alternatively, specify a CMake executable using the -x flag."
  exit 1
fi

#
# Remove assets in build directory if requested. This will force the
# build to re-copy them from the source tree into the build directory. 
#

if [ $RM_BUILD_ASSETS -eq 1 ]; then
    if [ $VERBOSE -eq 1 ]; then
        RM_VERBOSE_ARG="v"
        echo "Removing assets in ${BUILD_DIR}/assets"
    else
        RM_VERBOSE_ARG=""
    fi
    rm -rf${RM_VERBOSE_ARG} ${BUILD_DIR}/assets
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
        coretech/common/BUILD.in \
        coretech/common/clad/BUILD.in \
        coretech/vision/BUILD.in \
        coretech/vision/clad/BUILD.in \
        coretech/planning/BUILD.in \
        coretech/messaging/BUILD.in \
        cubeBleClient/BUILD.in \
        engine/BUILD.in \
        engine/tools/BUILD.in \
        lib/util/source/anki/util/BUILD.in \
        lib/util/source/anki/utilUnitTest/BUILD.in \
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
        #
        # If ANDROID_NDK is set, use it, else provide default location
        #
        if [ -z "${ANDROID_NDK+x}" ]; then
          ANDROID_NDK=`${TOPLEVEL}/tools/build/tools/ankibuild/android.py`
        fi

        PLATFORM_ARGS=(
            -DMACOSX=0
            -DANDROID=1
            -DANDROID_NDK="${ANDROID_NDK}"
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
        ${EXPORT_FLAGS} \
        ${FEATURE_FLAGS} \
        -DUSE_TENSORFLOW=${USE_TENSORFLOW} \
        -DUSE_TENSORFLOW_LITE=${USE_TENSORFLOW_LITE} \
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
  TARGET_ARG=""
  if [ -n "$CMAKE_TARGET" ]; then
    TARGET_ARG="--target $CMAKE_TARGET"
  fi
  $CMAKE_EXE --build . $TARGET_ARG $*
fi

popd > /dev/null 2>&1
