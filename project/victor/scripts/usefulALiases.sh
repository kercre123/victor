#!/usr/bin/env bash

# Handy Victor Aliases from Al Chaussee
# =====================================
# They can be called from anywhere within the git project to build, deploy, and run on physical robots.
# Call source on this file from your .bash_profile if you always want the latest version of these aliases 
# or copy its contents into your .bash_profile.

alias GET_GIT_ROOT='export GIT_PROJ_ROOT=`git rev-parse --show-toplevel`'

alias victor_restart='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_restart.sh'
alias victor_start='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_start.sh'
alias victor_stop='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_stop.sh'

# shipping, the build that is delivered to customers
# release, a shipping build with custom options for development, e.g. profilers, webservices

alias victor_build_shipping='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_build.sh -c Release'
alias victor_build_release='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_build.sh -c Release -DANKI_WEBSERVER_ENABLED=1'
alias victor_build_debug='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_build.sh -c Debug'
alias victor_build_debugo2='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_build.sh -c Debug -O2'
alias victor_build_xcode='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_build.sh -c Debug -p mac -g Xcode -C'

alias victor_deploy_release='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_deploy.sh -c Release'
alias victor_deploy_debug='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_deploy.sh -c Debug'
alias victor_deploy_run='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_deploy_run.sh'
alias victor_build_run='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_build_run.sh'

alias victor_log='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_log.sh'
alias victor_addr2line='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/addr2line.sh'

# If you have lnav...
alias victor_lnav='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/victor_log_lnav.sh'

alias vicos-which='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts/vicos_which.sh'
