# Victor Logging

## Logcat

Anki processes (and many system processes) log messages using the Android log interface `liblog`.
Log messages are stored in kernel memory and can be retrieved with an OS utility called `logcat`.
`Logcat` has options to filter by level and process tag.

Anki process logs are tagged with the process name (`vic-robot`, `vic-anim`, etc) to identify the source of each message.

Anki process logs use Android log levels to indicate the importance of each message, but we do NOT use the same naming!

* Anki ERROR = Android ERROR
* Anki WARNING = Android WARNING
* Anki EVENT = Android INFO
* Anki INFO = Android DEBUG
* Anki DEBUG = Android VERBOSE

## Victor Log Manager

By default, production robots do not save log messages to persistent storage, to maximize lifetime of the robot's
flash storage. Log messages are lost when the kernel buffer becomes full or the robot is rebooted.

Victor provides a log manager service (aka `vic-logmgr`) that can be used to save log messages to persistent storage.
This service is OFF by default but can be enabled for QA or developer tests.

If you have a command shell on the robot, you can enable or disable the service using `systemctl`:

``` bash
systemctl enable --now /anki/etc/systemd/system/vic-logmgr.service
systemctl disable --now vic-logmgr.service
```

If you are connected to the robot but do not have a command shell open, you can enable or disable the service using
helper scripts:

``` bash
cd project/victor/scripts
bash victor_enable_logmgr.sh
bash victor_disable_logmgr.sh
```

Once enabled, the log manager service will stay active until it is disabled.

## Victor Log Files

The log manager will create log files under `/data/data/com.anki.victor/cache/vic-logmgr`.
The current file is always named `vic-logmgr`.  Older files are named `vic-logmgr.0`, `vic-logmgr.1`, etc.
Files are automatically rotated by the log service.

Files may be copied off the robot using rsync, scp, [Android File Transfer](https://www.android.com/filetransfer)
or [Android Studio](https://developer.android.com/studio/index.html).

## Victor Log Parameters

Default log parameters will save a rolling window up to 64MB of data, including debug level from Victor processes
(`vic-robot`, `vic-anim`, etc) and error level from other system services.

Log parameters may be customized by editing `/anki/etc/vic-logmgr.env` on the robot.

The log manager script (`vic-logmgr.sh`) allows customization of the following parameters:

``` bash
VIC_LOGMGR_DIRECTORY="/data/data/com.anki.victor/cache/vic-logmgr"
VIC_LOGMGR_FILENAME="vic-logmgr"
VIC_LOGMGR_ROTATION_KB="1024"
VIC_LOGMGR_ROTATION_COUNT="64"
VIC_LOGMGR_FORMAT="printable"
VIC_LOGMGR_FILTERSPECS="vic-robot:V vic-anim:V vic-engine:V vic-webserver:V vic-cloud:V *:E"
```

See <https://developer.android.com/studio/command-line/logcat.html> for an explanation of filter and format options.

If you make changes in `vic-logmgr.env`, you must reboot the robot, or use a command shell to restart the service:

``` bash
systemctl restart vic-logmgr
```
