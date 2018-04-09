#!/usr/bin/env bash

export GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
${GIT_PROJ_ROOT}/project/victor/build-victor.sh "$@"

# Automatically build vicos too
# but only if we're building for physical robot
if [ $? -eq 0 ]; then
    # Remove any platform arguments
    args=("$@")
    for i in "${!args[@]}"
    do
	if [ "${args[i]}" = "-p" ]; then
        PLATFORM=${args[i+1]}
        PLATFORM=`echo $PLATFORM | tr "[:upper:]" "[:lower:]"`        
        if [ "${PLATFORM}" = "android" ]; then    
            echo "Automatically building VICOS"
            unset 'args[i]'
            unset 'args[i+1]'
        else
            exit 0
        fi
	fi
    done

    ${GIT_PROJ_ROOT}/project/victor/build-victor.sh -p vicos "${args[@]}"
fi
    
