## Fixture Code Repo


#  Fixture firmware dfu.

Prereqs
0. > adb wait-for-device
1. > adb root
2. > cd dfu
3. > make

Load New Image

1. > adb push fixture.safe /data/local/fixture/
2. > adb shell -x "cd /data/local/fixture && ./dfu fixture.safe"


