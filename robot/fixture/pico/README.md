Vector Encoder Tester
=====================

Setup:

1. Check out raspberry pi sdk: `git clone https://github.com/raspberrypi/pico-sdk --recursive`.
2. Set PICO_SDK_PATH: `export PICO_SDK_PATH=~/src/pico-sdk`
3. make a build folder so we don't pollute main contents. `cd THIS_DIR && mkdir build`
4. Run CMake setup. `cd build && cmake ../`
5. Build: `make`
6. Deploy to Pico: `picotool load enc_test.uf2 -f`

Watching realtime log:

1. `minicom -D /dev/cu.usbmodem123456`

Building fixture:

For now see the included Fritzing file.