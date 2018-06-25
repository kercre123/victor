#!/usr/bin/env python3

# copies tar and binary animations into a new test folder
import os
import re
import shutil
import tarfile

# This script assumes it is being called from ./project/buildServer/steps/unittestsEngine.sh
PROJECT_ROOT_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), '..', '..'))
EXTERNALS_DIR = os.path.join(PROJECT_ROOT_DIR, 'EXTERNALS')
INPUT_ANIMATION_PATH  = os.path.join(EXTERNALS_DIR, 'animation-assets/animations/')
OUTPUT_ANIMATION_PATH = os.path.join(PROJECT_ROOT_DIR, '_build/mac/Debug/data/assets/cozmo_resources/assets/dev_animation_data/')

if __name__ == "__main__":
    # clear out existing anim data
    if os.path.exists(OUTPUT_ANIMATION_PATH):
        shutil.rmtree(OUTPUT_ANIMATION_PATH)
    
    os.mkdir(OUTPUT_ANIMATION_PATH)

    # read in animation file names
    files = os.listdir(INPUT_ANIMATION_PATH)
    binary_files = [f for f in files if f[len(f) -4:] == ".bin"]
    tar_files = [f for f in files if f[len(f) -4:] == ".tar"]

    for file in binary_files:
        shutil.copy(INPUT_ANIMATION_PATH + file,  OUTPUT_ANIMATION_PATH + file)

    # Unpack tar files
    for file in tar_files:
        tar = tarfile.open(INPUT_ANIMATION_PATH + file)
        tar.extractall(INPUT_ANIMATION_PATH)
        tar.close()

    files = os.listdir(INPUT_ANIMATION_PATH)
    json_files = [f for f in files if f[len(f) -5:] == ".json"]
    for file in json_files:
        shutil.copy(INPUT_ANIMATION_PATH + file,  OUTPUT_ANIMATION_PATH + file)
