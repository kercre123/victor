README

Michelle Sintov
February 21, 2017

blocks/build.py is run to generate the following files:
    msg/js/en.js
    blockly_uncompressed_vertical.js
    blockly_uncompressed_horizontal.js
    blockly_compressed_vertical.js
    blockly_compressed_horizontal.js
    blocks_compressed_horizontal.js
    blocks_compressed_vertical.js
    blocks_compressed.js

build.py should be run after the contents of the blocks/ folder are changed. It compresses the core Blockly files into a single JavaScript file.

The compressed file is then used directly from index.html.

Docs from build.py:
# This script generates two versions of Blockly's core files:
# blockly_compressed.js
# blockly_uncompressed.js
# The compressed file is a concatenation of all of Blockly's core files which
# have been run through Google's Closure Compiler. This is done using the
# online API (which takes a few seconds and requires an Internet connection).
# The uncompressed file is a script that loads in each of Blockly's core files
# one by one. This takes much longer for a browser to load, but is useful
# when debugging code since line numbers are meaningful and variables haven't
# been renamed. The uncompressed file also allows for a faster developement
# cycle since there is no need to rebuild or recompile, just reload.
#
# This script also generates:
# blocks_compressed.js: The compressed common blocks.
# blocks_horizontal_compressed.js: The compressed Scratch horizontal blocks.
# blocks_vertical_compressed.js: The compressed Scratch vertical blocks.
# msg/js/<LANG>.js for every language <LANG> defined in msg/js/<LANG>.json.

Instructions to use build.py are on the following link (and are better than the instructions found at developers.google.com/blockly/guides/modify/web/closure):
https://github.com/LLK/scratch-blocks/wiki

By following the instructions, you will create a folder named closure-library (over 80 MB in size) that is a sibling to the blocks/ folder.

The current steps for me are as follows:
  cd cozmo-one/unity/Cozmo/Assets/StreamingAssets/Scratch/lib
  rm -rf closure-library
  cd blocks
  ln -s $(npm root)/google-closure-library ../closure-library
  npm install

Web browser debugging tip: by having the closure-library folder in place, you can load index.html in your desktop browser and debug the contents of the webview.
