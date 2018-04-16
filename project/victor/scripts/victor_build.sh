#!/usr/bin/env bash

export GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
${GIT_PROJ_ROOT}/project/victor/build-victor.sh "$@"

# Build vicos
if [ $? -eq 0 ]; then
    # Remove any platform arguments
    args=("$@")
    for i in "${!args[@]}"
    do
	if [[ "${args[i]}" = "-p" ]]; then
	    unset 'args[i]'
	    unset 'args[i+1]'
	fi
    done

    ${GIT_PROJ_ROOT}/project/victor/build-victor.sh -p vicos "${args[@]}"
fi
    
