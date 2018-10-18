# Victor Logging

Note: The best source of information about our various logging macros is [logging.h](/lib/util/source/anki/util/logging/logging.h) in Util.

### Logging isn't free

Writing a log statement costs system resources since it involves IPC messaging to a logging process. Excessive logging hogs resources and causes system-wide slowdown, resulting in subpar robot performance.

Log statements should thus never be spammy or repetitive, and should be used thoughtfully and sparingly. They should be well named so that anyone (developers or otherwise) can quickly understand what's happening and why it's worthy of a log statement.

### Log levels

It's important to select the appropriate log level for your message. Here are some rough guidelines:

- `ERROR` indicates that something has gone so wrong that your module cannot continue normal execution anymore. In [Webots tests](/simulator/controllers/webotsCtrlBuildServerTest/README.md), any error message appearing in the log will cause the test to fail.
- `WARNING` indicates a problem has occurred that users/developers/QA need to know about. A warning points to a situation that _should_ not happen and that should be fixed, but is not severe enough to warrant an error.
- `INFO` is for everyone to see. It should contain information useful to non-developers.
- `DEBUG` is for development. Debug statements include technical information or raw data not valuable to anyone except the developer of the module and other stakeholders in that module.

Note that `DEBUG` log statements get compiled out in `release` and `shipping` build configurations.

### Log channels

Log channels are simple strings that provide a facility to filter logs, enabling select channels and disabling others based on the type of problem you are interested in debugging. Each high-level code module will usually use its own log channel (e.g. `Behaviors`, `Actions`, `BlockWorld`, `Planner`).

Using log channels is preferable to using "named" prints (e.g. `PRINT_NAMED_INFO`), which are much more difficult to filter.

Note: Log channels apply only to debug and info levels. Warning and error levels are always `NAMED`.

### Best practices

The preferred way to write logs is to declare a log channel using `#define LOG_CHANNEL "MyLogChannel"`, and use the `LOG_DEBUG` and `LOG_INFO` macros. Here's an example:

```cpp
  // myFile.cpp

  #define LOG_CHANNEL "MyLogChannel"

  // ...

  // LOG_DEBUG will print a debug message using the defined LOG_CHANNEL above.
  // No duplication of channel name!
  LOG_DEBUG("SomeDebugStuff", "Some useful information for developers: ...");

  LOG_DEBUG("MoreDebugStuff", "More useful info");

  LOG_INFO("SomeInfo", "An event happened that everyone should know about");
```

## Logging system details

### Logcat

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

### Victor Log Manager

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

### Victor Log Files

The log manager will create log files under `/data/data/com.anki.victor/cache/vic-logmgr`.
The current file is always named `vic-logmgr`.  Older files are named `vic-logmgr.0`, `vic-logmgr.1`, etc.
Files are automatically rotated by the log service.

Files may be copied off the robot using rsync, scp, [Android File Transfer](https://www.android.com/filetransfer)
or [Android Studio](https://developer.android.com/studio/index.html).

### Victor Log Parameters

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
