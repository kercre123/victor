#!/usr/bin/env bash

# Handy Victor Audio Aliases from Jordan Rivas, inspired by Al Chaussee
# =====================================
# They can be called from anywhere within the git project to build, deploy, and run on physical robots.
# Call source on this file from your .bash_profile if you always want the latest version of these aliases 
# or copy its contents into your .bash_profile.
#
# NOTE: Must add VICTOR_WWISE_PROJECT_TRUNK_PATH to ~/.bash_profile above calling source on this file

alias GET_GIT_ROOT='export GIT_PROJ_ROOT=`git rev-parse --show-toplevel`'

# Update Local assets for Mac, Robot & Maya. This always allows assets to be missing, Sound Designers have the power!
alias victor_audio_update_local_assets='GET_GIT_ROOT; ${GIT_PROJ_ROOT}/tools/audio/UseLocalAudioAssets.py $VICTOR_WWISE_PROJECT_TRUNK_PATH  --allow-missing'
# Robot (VICOS) Build & Deploy
alias victor_audio_robot_build_deploy_update_local_assets='GET_GIT_ROOT; victor_audio_update_local_assets -c Release -b'
# Mac (Webots) Build
alias victor_audio_webots_build_update_local_assets='GET_GIT_ROOT; victor_audio_update_local_assets -p mac -c Release -b'

# Clean assset directories
alias victor_audio_clean_local_assets='GET_GIT_ROOT; rm -rf ${GIT_PROJ_ROOT}/EXTERNALS/local-audio-assets'
alias victor_audio_clean_project_assets='GET_GIT_ROOT; rm -rf $VICTOR_WWISE_PROJECT_TRUNK_PATH/VictorAudio/GeneratedSoundBanks'
