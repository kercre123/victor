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
    echo "  -p [PLATFORM]           build target platform {mac,vicos}"
    echo "  -a                      append cmake platform argument {arg}"
    echo "  -g [GENERATOR]          CMake generator {Ninja,Xcode,Makefiles}"
    echo "  -f                      force-run filelist updates and cmake configure before building"
    echo "  -X                      delete build assets, forcing assets to be re-copied"
    echo "  -d                      DEBUG: generate file lists and exit"
    echo "  -x [CMAKE_EXE]          path to cmake executable"
    echo "  -C                      generate build config and exit without building"
    echo "  -D                      allow #defines to be specified from the command-line"
    echo "  -F [FEATURE]            enable feature {factoryTest,factoryTestDev}"
    echo "  -T                      list all cmake targets"
    echo "  -t [target]             build specified cmake target"
    echo "  -e                      export compile commands"
    echo "  -I                      ignore external dependencies"
    echo "  -S                      build static libraries"
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
EXPORT_COMPILE_COMMANDS=0
IGNORE_EXTERNAL_DEPENDENCIES=0
BUILD_SHARED_LIBS=1

CONFIGURATION=Debug
PLATFORM=vicos
GENERATOR=Ninja
FEATURES=""
DEFINES=""
ADDITIONAL_PLATFORM_ARGS=()

while getopts ":x:c:p:a:t:g:F:D:hvfdCTeISX" opt; do
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
        D)
            # -D defines on the command-line will force a reconfigure, save any dev gotchas
            CONFIGURE=1
            DEFINES="${DEFINES} -D${OPTARG}"
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
        a)
            ADDITIONAL_PLATFORM_ARGS+=("${OPTARG}")
            ;;
        g)
            GENERATOR="${OPTARG}"
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
        I)
            IGNORE_EXTERNAL_DEPENDENCIES=1
            ;;
        S)
            BUILD_SHARED_LIBS=0
            ;;
        X)
            RM_BUILD_ASSETS=1
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
# Run everything from ${TOPLEVEL}, even if invoked from somewhere else
cd ${TOPLEVEL}

#
# settings
#

if [ -z "${CMAKE_EXE+x}" ]; then
    echo "Attempting to install cmake"
    ${TOPLEVEL}/tools/build/tools/ankibuild/cmake.py --install-cmake 3.9.6
    CMAKE_EXE=`${TOPLEVEL}/tools/build/tools/ankibuild/cmake.py --find-cmake 3.9.6`
fi

if [ $IGNORE_EXTERNAL_DEPENDENCIES -eq 0 ]; then
  echo "Attempting to run fetch-build-deps.sh"
  ${TOPLEVEL}/project/victor/scripts/fetch-build-deps.sh
else
  echo "Ignore external dependencies"
fi

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
# Validate generator
#
case "${GENERATOR}" in
  [Nn][Ii][Nn][Jj][Aa])
    GENERATOR="Ninja"
    ;;
  [Xx][Cc][Oo][Dd][Ee])
    GENERATOR="Xcode"
    ;;
  [Mm][Aa][Kk][Ee][Ff][Ii][Ll][Ee][Ss])
    GENERATOR="Makefiles"
    ;;
  "Unix Makefiles")
    GENERATOR="Makefiles"
    ;;
  *)
    echo "${SCRIPT_NAME}: Unknown generator '${GENERATOR}'"
    usage
    exit 1
    ;;
esac

#
# Enable feature flags
#
FEATURE_FLAGS="-DFACTORY_TEST=0"

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
# Get short commit sha
#
export ANKI_BUILD_SHA=`git rev-parse --short HEAD`

#
# Enable export flags
#
EXPORT_FLAGS=""
if [ ${EXPORT_COMPILE_COMMANDS} -ne 0 ]; then
  EXPORT_FLAGS="-DCMAKE_EXPORT_COMPILE_COMMANDS=${EXPORT_COMPILE_COMMANDS}"
fi

# For non-ninja builds, add generator type to build dir
BUILD_SYSTEM_TAG=""
if [ "${GENERATOR}" != "Ninja" ]; then
    BUILD_SYSTEM_TAG="-${GENERATOR}"
fi
: ${BUILD_DIR:="${TOPLEVEL}/_build/${PLATFORM}/${CONFIGURATION}${BUILD_SYSTEM_TAG}"}

case ${GENERATOR} in
    "Ninja")
        PROJECT_FILE="build.ninja"
        ;;
    "Xcode")
        PROJECT_FILE="victor.xcodeproj"
        ;;
    "Makefiles")
        PROJECT_FILE="Makefile"
        GENERATOR="CodeBlocks - Unix Makefiles"
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
    echo "Unsupported CMake generator: ${GENERATOR}"
    exit 1
fi

: ${CMAKE_MODULE_DIR:="${TOPLEVEL}/cmake"}

if [ ! -f ${CMAKE_EXE} ]; then
  echo "Missing CMake executable: ${CMAKE_EXE}"
  echo "Fetch the required CMake version by running ${TOPLEVEL}/tools/build/tools/ankibuild/cmake.py"
  echo "Alternatively, specify a CMake executable using the -x flag."
  exit 1
fi

if [ -z "${GOROOT+x}" ]; then
    GO_EXE=`${TOPLEVEL}/tools/build/tools/ankibuild/go.py`
    export GOROOT=$(dirname $(dirname $GO_EXE))
else
    GO_EXE=$GOROOT/bin/go
fi
export GOPATH=${TOPLEVEL}/cloud/go:${TOPLEVEL}/generated/cladgo:${TOPLEVEL}/generated/go:${TOPLEVEL}/tools/message-buffers/support/go

if [ ! -f ${GO_EXE} ]; then
  echo "Missing Go executable: ${GO_EXE}"
  echo "Fetch the required Go version by running ${TOPLEVEL}/tools/build/tools/ankibuild/go.py"
  exit 1
fi

${TOPLEVEL}/tools/build/tools/ankibuild/go.py --check-version $GO_EXE

PROTOC_EXE=`${TOPLEVEL}/tools/build/tools/ankibuild/protobuf.py --install | tail -1`
PROTOBUF_HOME=`cd $(dirname "${PROTOC_EXE}")/.. && pwd`

#
# Remove assets in build directory if requested. This will force the
# build to re-copy them from the source tree into the build directory.
#

if [ $RM_BUILD_ASSETS -eq 1 ]; then
    if [ $VERBOSE -eq 1 ]; then
        RM_VERBOSE_ARG="v"
        echo "Removing assets in ${BUILD_DIR}/data/assets"
    else
        RM_VERBOSE_ARG=""
    fi
    rm -rf${RM_VERBOSE_ARG} ${BUILD_DIR}/data/assets
fi

#
# grab Go dependencies ahead of generating source lists
#
# needed for metabuild if we have to run it
if [ $IGNORE_EXTERNAL_DEPENDENCIES -eq 0 ] || [ $CONFIGURE -eq 1 ] ; then
    GEN_SRC_DIR="${TOPLEVEL}/generated/cmake"
    if [ $CONFIGURE -eq 1 ] ; then
      rm -rf "${GEN_SRC_DIR}"
    fi
    mkdir -p "${GEN_SRC_DIR}"

    # Scan for BUILD.in files
    METABUILD_INPUTS=`find . -name BUILD.in`
fi

if [ $IGNORE_EXTERNAL_DEPENDENCIES -eq 0 ]; then
  echo "Getting Go dependencies"
  # Process BUILD.in files (creates list of Go projects to fetch)
  ${BUILD_TOOLS}/metabuild/metabuild.py --go-output \
      -o ${GEN_SRC_DIR} \
      ${METABUILD_INPUTS}
  # Check out specified revisions of repositories we've versioned
  # Append a dummy dir to the GOPATH so that `go get` doesn't barf
  # on nonexistent clad files
  GODUMMY=${TOPLEVEL}/cloud/dummy
  (cd ${TOPLEVEL}; PATH="$PATH:$(dirname $GO_EXE)" GOPATH="$GOPATH:$GODUMMY" ./godeps.js execute ${GEN_SRC_DIR})
else
  echo "Ignore Go dependencies"
fi

#
# generate source file lists
#

if [ $CONFIGURE -eq 1 ]; then
    mkdir -p ${BUILD_DIR}

    if [ $VERBOSE -eq 1 ]; then
        METABUILD_VERBOSE="-v"
    else
        METABUILD_VERBOSE=""
    fi

    # Process BUILD.in files
    ${BUILD_TOOLS}/metabuild/metabuild.py $METABUILD_VERBOSE \
        -o ${GEN_SRC_DIR} \
        ${METABUILD_INPUTS}

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
            -DVICOS=0
            -DCMAKE_TOOLCHAIN_FILE="${CMAKE_MODULE_DIR}/macosx.toolchain.cmake"
        )
    elif [ "$PLATFORM" == "vicos" ] ; then
        #
        # If VICOS_SDK is set, use it, else provide default location
        #
        if [ -z "${VICOS_SDK+x}" ]; then
            VICOS_SDK=$(${TOPLEVEL}/tools/build/tools/ankibuild/vicos.py --install 0.9-r03 | tail -1)
        fi
 
        PLATFORM_ARGS=(
            -DMACOSX=0
            -DANDROID=0
            -DVICOS=1
            -DVICOS_SDK="${VICOS_SDK}"
            -DCMAKE_TOOLCHAIN_FILE="${CMAKE_MODULE_DIR}/vicos.oelinux.toolchain.cmake"
            -DVICOS_CPP_FEATURES='rtti exceptions'
        )
        echo "${PLATFORM_ARGS[@]}"
    else
        echo "unknown platform: ${PLATFORM}"
        exit 1
    fi

    # Append additional platrom args
    PLATFORM_ARGS+=(${ADDITIONAL_PLATFORM_ARGS[@]})
    $CMAKE_EXE ${TOPLEVEL} \
        ${VERBOSE_ARG} \
        -G"${GENERATOR}" \
        -DCMAKE_BUILD_TYPE=${CONFIGURATION} \
        -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS} \
        -DGOPATH=${GOPATH} \
        -DGOROOT=${GOROOT} \
        -DPROTOBUF_HOME=${PROTOBUF_HOME} \
        -DANKI_BUILD_SHA=${ANKI_BUILD_SHA} \
        ${EXPORT_FLAGS} \
        ${FEATURE_FLAGS} \
        ${DEFINES} \
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
  if [ "$PLATFORM" != "mac" ]; then
    # run install target on robot-platforms
    $CMAKE_EXE --build . --target install
  fi
fi

popd > /dev/null 2>&1
