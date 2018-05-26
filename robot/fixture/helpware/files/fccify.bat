adb wait-for-device
adb shell "mount -o rw,remount / ; rm /anki/menuman"
adb shell "mv /usr/bin/ankibluetoothd /usr/bin/ankiblues ; systemctl disable vic-engine ; systemctl disable vic-anim ; systemctl disable vic-robot ; systemctl disable anki-robot.target ; systemctl disable vic-watchdog"
adb push ankiinit /etc/initscripts
adb push square-05.wav sine-05.wav testsound.sh burn menuman system.sh menuman.sh burn.sh /anki
adb shell "chmod +x /anki/* ; chmod +x /etc/initscripts/ankiinit ; sync ; reboot"
