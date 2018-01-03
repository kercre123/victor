Assumes some kernel is loaded.

# Load latest syscon:  Verifies communication:

```
cd robot
vmake
./dfu.sh   
```

# Load and run latest robot code
```
./project/victor/build_victor.sh
./project/victor/scripts/deploy_assets.sh
./project/victor/scripts/deploy.sh

adb shell
> 
 /data/data/com.anki.cozmoengine/robotctl.sh restart
 /data/data/com.anki.cozmoengine/cozmoctl.sh restart


```
Monitor `adb_logcat`.

Shut down with
```
/data/data/com.anki.cozmoengine/robotctl.sh restart
/data/data/com.anki.cozmoengine/cozmoctl.sh restart

```


# confirm video


```

cd robot/test
make lcd_test && adb push lcd_test /data/local/tmp
adb push video.gif.raw /data/local/tmp
adb shell
>
 cd /data/local/tmp
 chmod a+x lcd_test
 ./lcd_test video.gif.raw
```


To create a raw file from an animated gif:
```
cd robot/test
python3 gif_to_raw.py video.gif
```
To create an animated gif from a series of images (imagemagik must be installed):

```
convert -loop 0  *.png video.gif
```

#confirm audio

```
cd robot/test/audio
make && adb push alsaplay /data/local/tmp
adp push song.wav /data/local/tmp
adb shell
>
 cd /data/local/tmp
 tinymix "PRI_MI2S_RX Audio Mixer MultiMedia1" 1 
 tinymix "RX3 MIX1 INP1" "RX1"
 tinymix "SPK DAC Switch" 1
 ./alsaplay song.wav
```


# confirm camera

```
cd androidHAL/android/proto_camera/test
make && adb push camera_test /data/local/tmp
adb shell
>
 rm -f /data/misc/camera/test/main* && /data/local/tmp/camera_test
 ^C
 ^D
rm -f test/main* && adb pull /data/misc/camera/test/ . && cd test && ./pp.sh && cd ..
```
