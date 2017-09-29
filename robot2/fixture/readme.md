# Fixture Code Repo


##  Fixture firmware dfu.

### Make dfu app
1. > cd dfu
2. > make

### Load dfu app

1. > adb wait-for-device
3. > make push

### #run dfu app

1. > adb push fixture.safe /data/local/fixture/
2. > make execute safe=fixture.safe


