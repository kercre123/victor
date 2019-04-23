# PerfMetric Tool

PerfMetric is a tool that is compiled into Victor's engine and anim processes, and records information about each tick in a circular buffer in memory. After a short recording session, one can examine the buffer of recorded tick information. PerfMetric's CPU overhead is minimal. PerfMetric is compiled out of shipping builds.

Some uses:
* See how spikey the engine or anim tick rate is
* See that certain behaviors or activities use more or less CPU
* Measure before-and-after average tick durations, for optimization attempts
* Compare Debug vs. Release performance
* Identify extra-long ticks which might indicate some initialization code that could be optimized
* Use with automated performance testing

What is recorded for each vic-engine tick:
* Corresponding robot time which can be used to compare this information with logs
* Engine duration: The time the engine tick took to execute, in ms
* Engine frequency: The total time of the engine tick, including sleep, in ms
* Sleep intended: The duration the engine intended to sleep, in ms
* Sleep actual: The duration the engine actually slept, in ms; this is often more than what was intended
* Oversleep: How much longer the engine slept than intended, in ms
* RtE Count: How many robot-to-engine messages were received
* EtR Count: How many engine-to-robot messages were sent
* GtE Count: How many game-to-engine messages were received
* EtG Count: How many engine-to-game messages were sent
* GWtE Count: How many gateway-to-engine messages were received
* EtGW Count: How many engine-to-gateway messages were sent
* Viz Count: How many vizualization messages were sent
* Battery Voltage: Current battery voltage (at one point we thought this might be helpful in debugging)
* CPU Freq: CPU frequency (Note: not updated every tick, so a change typically appears several ticks later)
* Active Feature: The current 'active feature'
* Behavior: The current top-of-stack behavior

What is recorded for each vic-anim tick:
* Corresponding robot time which can be used to compare this information with logs
* Anim duration: The time the anim tick took to execute, in ms
* Anim frequency: The total time of the anim tick, including sleep, in ms
* Sleep intended: The duration the anim process intended to sleep, in ms
* Sleep actual: The duration the anim process actually slept, in ms; this is often more than what was intended
* Oversleep: How much longer the anim process slept than intended, in ms
* RtA Count: How many robot-to-anim messages were received
* AtR Count: How many anim-to-robot messages were sent
* EtA Count: How many engine-to-anim messages were received
* AtE Count: How many anim-to-engine messages were sent
* Anim Time: The time offset within the current animation, in ms; hard-incremented by 33 ms each frame
* Layer Count: The number of composite layers being rendered

When results are dumped, a summary section shows extra information, including the mininum, maximum, average, and standard deviation for each of the appropriate stats. This allows you to see, for example, the average tick duration, or the longest tick.

### Sample output (for vic-engine)

```
                      Engine   Engine    Sleep    Sleep     Over      RtE   EtR   GtE   EtG  GWtE  EtGW   Viz  Battery    CPU
                    Duration     Freq Intended   Actual    Sleep    Count Count Count Count Count Count Count  Voltage   Freq  Active Feature/Behavior
 13:07:12.076     0  110.063  110.181    0.000    0.118    0.118       26     2     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.188     1   11.051   11.154    0.000    0.103    0.103        0     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.200     2   15.981   59.057   42.117   43.075    0.958        4     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.260     3   12.284   59.879   46.756   47.595    0.839        6     0     0     4     0     2     0    3.585 200000  Sleeping  Asleep
 13:07:12.320     4   15.335   60.018   43.826   44.682    0.856       10     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.380     5   14.099   59.964   45.044   45.865    0.821        9     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.440     6   12.435   63.838   46.743   51.403    4.660        7     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.504     7   16.663   56.356   38.676   39.693    1.017       13     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.560     8   15.083   61.063   43.899   45.980    2.081        6     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.620     9   22.696   58.712   35.222   36.015    0.793        9     2     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.680    10   12.696   59.890   46.510   47.193    0.683        9     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.740    11   13.494   61.002   45.822   47.508    1.686       10     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.800    12   12.743   60.781   45.569   48.037    2.468       10     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.860    13   13.110   58.175   44.421   45.065    0.644        7     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.920    14   19.807   62.536   39.548   42.729    3.181        7     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:12.984    15   14.361   57.213   42.457   42.852    0.395       10     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 13:07:13.040    16   15.967   60.214   43.637   44.247    0.610       10     0     0     2     0     1     0    3.585 200000  Sleeping  Asleep
 Summary:  (RELEASE build; VICOS; 17 ticks; 1.020 seconds total)
                      Engine   Engine    Sleep    Sleep     Over      RtE   EtR   GtE   EtG  GWtE  EtGW   Viz  Battery    CPU
                    Duration     Freq Intended   Actual    Sleep    Count Count Count Count Count Count Count  Voltage   Freq
               Min:   11.051   11.154    0.000    0.103    0.103      0.0   0.0   0.0   2.0   0.0   1.0   0.0    3.585 200000
               Max:  110.063  110.181   46.756   51.403    4.660     26.0   2.0   0.0   4.0   0.0   2.0   0.0    3.585 200000
              Mean:   20.463   60.002   38.250   39.539    1.289      9.0   0.2   0.0   2.1   0.0   1.1   0.0    3.585 200000
               Std:   22.575   17.072   14.286   14.785    1.163      5.1   0.6   0.0   0.5   0.0   0.2   0.0    0.000      0
```

### Prerequisites
PerfMetric is compiled into the engine and anim processes by default for Debug and Release builds, but not for Shipping builds. To override the default, use

```-DANKI_PERF_METRIC_ENABLED=1```

(use 0 to disable)

### Use from command line
The interface to PerfMetric is through Victor's embedded web server. From the command line, this will start a recording session for vic-engine, assuming ANKI_ROBOT_HOST is set to your robot IP (use port 8889 for vic-anim):

```curl ${ANKI_ROBOT_HOST}':8888/perfmetric?start'```

You can also use your robot IP directly, this way:

```curl '192.168.42.82:8888/perfmetric?start'```

Now some time later, one can dump the results with:

```curl ${ANKI_ROBOT_HOST}':8888/perfmetric?stop&dumplogall'```

Note that multiple commands can be entered at the same time, separated by ampersands. This includes some 'wait' commands that allow you to do a recording session with one single command:

```curl ${ANKI_ROBOT_HOST}':8888/perfmetric?start&waitseconds30&stop&dumplogall'```

Here is the complete list of commands and what they do:
* "status" returns status (recording or stopped, and number of frames in buffer)
* "start" starts recording; if a recording was in progress, the buffer is reset before re-starting
* "stop" stops recording
* "dumplog" dumps the summary of results to the log
* "dumplogall" dumps the entire recorded tick buffer, along with the summary, to the log
* "dumpresponse" returns summary as HTTP response
* "dumpresponseall" returns all info as HTTP response
* "dumpfiles" writes all info to two files on the robot: One is a formatted txt file, and the other a csv file. These go in /data/data/com.anki.victor/cache/perfMetricLogs. The filename has the time of the file write baked in, as well as "R" or "D" to indicate Release or Debug build, and "Eng" or "Anim" to indicate vic-engine or vic-anim. An example: "perfMetric_2018-11-29_17-41-05_R_Eng.txt".  Only the last 50 files are kept, to prevent running out of disk space on the robot.
* "waitticksN", where N is an integer, waits for that number of ticks before proceeding to the next of these commands.
* "waitsecondsN", where N is a float number, waits for that number of seconds before proceeding to the next of these commands.

### Use from webserver page in a browser
The engine (port 8888) and anim (port 8889) webserver pages have a "PERF METRIC" page button. This brings you to a page with all of the PerfMetric controls, including the ability to dump the formatted output to the page itself.

There is also a graph feature that can be used to see a graph of any combination of 1 to 3 of the recorded per-tick pieces of data.  To use, first either click 'Dump all' or 'Toggle Graph Auto Update' to get some data.  Next, use one of the "Data Selectors" pull-down menus (which get populated only after the first dump) to select one of the data categories.  Then click "Render Graph".  If you had toggled on 'graph auto update' then the graph will continually update and scroll over time.

### Helper script
A script has been created for convenience and as an example:

```tools/perfMetric/autoPerfMetric.sh```

### Use with webots (pure simulator)
When using with webots pure simulator, use 'localhost' as your IP. Also note that webots pure simulator does NOT sleep between engine ticks, so the output will reflect webots' fake sleep.

### Size of circular buffer in memory
The number of frames stored in the circular buffer is defined with `kNumFramesInBuffer` and is 1000 for vic-engine, and 2000 for vic-anim.  The difference is because vic-anim's target tick duration is 33 ms and vic-engine's target tick duration is 60 ms.  The total size in memory of vic-engine's PerfMetric buffer is about 86 KB.  The size of vic-anim's buffer is about 78 KB.

