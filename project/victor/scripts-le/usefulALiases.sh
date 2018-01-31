# Handy Victor Aliases from Al Chaussee
# =====================================
# They can be called from anywhere within the git project to build, deploy, and run on physical robots.
# Call source on this file from your .bash_profile if you always want the latest version of these aliases 
# or copy its contents into your .bash_profile.

alias GET_GIT_ROOT='export GIT_PROJ_ROOT=`git rev-parse --show-toplevel`'

alias victor_restart='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts-le/robotctl.sh restart && sleep 5 &&  ${GIT_PROJ_ROOT}/project/victor/scripts-le/animctl.sh restart && sleep 5 && ${GIT_PROJ_ROOT}/project/victor/scripts-le/cozmoctl.sh restart && ${GIT_PROJ_ROOT}/project/victor/scripts-le/cloudctl.sh restart'
alias victor_stop='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts-le/cozmoctl.sh stop && ${GIT_PROJ_ROOT}/project/victor/scripts-le/robotctl.sh stop && ${GIT_PROJ_ROOT}/project/victor/scripts-le/animctl.sh stop && ${GIT_PROJ_ROOT}/project/victor/scripts-le/cloudctl.sh stop'

alias victor_build='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/build-victor.sh -c Release'
alias victor_deploy='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts-le/deploy.sh -c Release'
alias victor_deploy_run='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts-le/deploy.sh -c Release; victor_restart'
alias victor_assets='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts-le/deploy-assets.sh ${GIT_PROJ_ROOT}/_build/android/Release/assets'
alias victor_assets_force='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/project/victor/scripts-le/deploy-assets.sh -f ${GIT_PROJ_ROOT}/_build/android/Release/assets'
alias victor_log='adb logcat mm-camera:S mm-camera-intf:S mm-camera-eztune:S mm-camera-sensor:S mm-camera-img:S cnss-daemon:S cozmoengined:S ServiceManager:S chatty:S'
alias victor_ble='GET_GIT_ROOT; node ${GIT_PROJ_ROOT}/tools/victor-ble-cli/index.js'

# If you have lnav...
alias victor_lnav='adb logcat | lnav -c '\'':filter-out mm-camera'\'' -c '\'':filter-out cnss-daemon'\'' -c '\'':filter-out ServiceManager'\'' -c '\'':filter-out chatty'\'''
