function victorhelp() {
cat <<EOF
Invoke ". project/victor/envsetup.sh" from your shell to add the following functions to your environment:

- croot:    Changes directory to the top of the tree.
- cgrep:    Greps on all local C/C++ files.
- mgrep:    Greps on all local Make/CMake files.
- godir:    Go to the directory containing a file.
EOF
}

# NOTICE: The following functions are from AOSP build/envsetup.sh
# gettop, croot, cgrep, mgrep, godir
# Licensed under Apache 2.0


# return the "top" folder
# For this repo, we define TOP to be the folder that contains: 
# project/victor/build-victor.sh
function gettop
{
    local TOPFILE=project/victor/build-victor.sh
    if [ -n "$TOP" -a -f "$TOP/$TOPFILE" ] ; then
        # The following circumlocution ensures we remove symlinks from TOP.
        (cd $TOP; PWD= /bin/pwd)
    else
        if [ -f $TOPFILE ] ; then
            # The following circumlocution (repeated below as well) ensures
            # that we record the true directory name and not one that is
            # faked up with symlink names.
            PWD= /bin/pwd
        else
            local HERE=$PWD
            T=
            while [ \( ! \( -f $TOPFILE \) \) -a \( $PWD != "/" \) ]; do
                \cd ..
                T=`PWD= /bin/pwd -P`
            done
            \cd $HERE
            if [ -f "$T/$TOPFILE" ]; then
                echo $T
            fi
        fi
    fi
}

#
# cd helper functions
#

function croot()
{
    T=$(gettop)
    if [ "$T" ]; then
        \cd $(gettop)
    else
        echo "Couldn't locate the top of the tree.  Try setting TOP."
    fi
}

function godir () {
    if [[ -z "$1" ]]; then
        echo "Usage: godir <regex>"
        return
    fi
    T=$(gettop)
    if [ ! "$OUT_DIR" = "" ]; then
        mkdir -p $OUT_DIR
        FILELIST=$OUT_DIR/filelist
    else
        FILELIST=$T/filelist
    fi
    if [[ ! -f $FILELIST ]]; then
        echo -n "Creating index..."
        (\cd $T; find . -wholename ./out -prune -o -wholename ./.repo -prune -o -wholename ./.git -prune -o -wholename ./_build -prune -o -wholename ./build -prune -o -type f > $FILELIST)
        echo " Done"
        echo ""
    fi
    local lines
    lines=($(\grep "$1" $FILELIST | sed -e 's/\/[^/]*$//' | sort | uniq))
    if [[ ${#lines[@]} = 0 ]]; then
        echo "Not found"
        return
    fi
    local pathname
    local choice
    if [[ ${#lines[@]} > 1 ]]; then
        while [[ -z "$pathname" ]]; do
            local index=1
            local line
            for line in ${lines[@]}; do
                printf "%6s %s\n" "[$index]" $line
                index=$(($index + 1))
            done
            echo
            echo -n "Select one: "
            unset choice
            read choice
            if [[ $choice -gt ${#lines[@]} || $choice -lt 1 ]]; then
                echo "Invalid choice"
                continue
            fi
            pathname=${lines[$(($choice-1))]}
        done
    else
        pathname=${lines[0]}
    fi
    \cd $T/$pathname
}

#
# grep helper functions
#

function cgrep()
{
    find . -name .repo -prune -o -name .git -prune -o -name out -prune -o -name _build -prune -o -name build -prune -o -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) \
        -exec grep --color -n "$@" {} +
}

case `uname -s` in
    Darwin)
        function mgrep()
        {
            find -E . -name .repo -prune -o -name .git -prune -o -path ./out -prune -o -path ./_build -prune -o -path ./build -prune -o -type f -iregex '.*/(CMakeLists.txt|Makefile|Makefile\..*|.*\.make|.*\.mak|.*\.mk|.*\.cmake)' \
                -exec grep --color -n "$@" {} +
        }
        ;;
    *)
        function mgrep()
        {
            find . -name .repo -prune -o -name .git -prune -o -path ./out -prune -o -path ./_build -prune -o -path ./build -prune -o -regextype posix-egrep -iregex '(.*\/CMakeLists.txt|.*\/Makefile|.*\/Makefile\..*|.*\.make|.*\.mak|.*\.mk|.*\.cmake)' -type f \
                -exec grep --color -n "$@" {} +
        }
        ;;
esac

pathupdate()
{
    case ":${PATH:=$1}:" in
        *:$1:*)
            ;;
        *) PATH="$1:$PATH"
            ;;
    esac
}

#
# If cmake is not already on $PATH, find it and add it
#
function set_cmake_env()
{
    local cmake="`which cmake`"
    if [ -z ${cmake} ]; then
        local toplevel=$(gettop)
        cmake="`${toplevel}/tools/build/tools/ankibuild/cmake.py`"
        cmake=$(dirname ${cmake})
        pathupdate ${cmake}
    fi
}

function set_android_env()
{
    local TOPLEVEL=$(gettop)
    echo ${TOPLEVEL}
    if [ -e $TOPLEVEL/local.properties ]; then
        export ANDROID_HOME=`egrep sdk.dir $TOPLEVEL/local.properties | awk -F= '{print $2;}'`
        export ANDROID_SDK=${ANDROID_HOME}
        pathupdate "${ANDROID_HOME}/bin"
        pathupdate "${ANDROID_HOME}/tools/bin"
        pathupdate "${ANDROID_HOME}/platform-tools"

        export ANDROID_NDK_HOME=`egrep ndk.dir $TOPLEVEL/local.properties | awk -F= '{print $2;}'`
        export ANDROID_NDK_ROOT=${ANDROID_NDK_HOME}
        export ANDROID_NDK=${ANDROID_NDK_HOME}
        pathupdate "${ANDROID_NDK_HOME}"
    fi
}

# setup cmake environment unless SKIP_CMAKE_SETUP is set
if [ -z ${SKIP_CMAKE_SETUP+x} ]; then
    set_cmake_env
fi

# setup android environment unless SKIP_ANDROID_SETUP is set
if [ -z ${SKIP_ANDROID_SETUP+x} ]; then
    set_android_env
fi
