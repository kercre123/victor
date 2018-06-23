# PerfMetric Tool

PerfMetric is a tool that is optionally compiled into Victor's engine, that records information about each engine tick.  After a short recording session, one can examine the buffer of recorded ticks.  This tool was used in Cozmo.  PerfMetric's CPU overhead is minimal.

Some uses:
* See how spikey or smooth the engine tick rate is
* See that certain behaviors or activities use more or less engine CPU
* Measure before-and-after average tick rates, for optimization attempts
* Compare Debug vs. Release performance
* For automated peformance testing

What is recorded for each tick:
* Engine duration: The time the engine tick took to execute, in ms
* Engine frequency: The total time of the engine tick, including sleep, in ms
* Sleep intended: The duration the engine intended to sleep, in ms
* Sleep actual: The duration the engine actually slept, in ms; this is often more than what was intended
* Oversleep: How much longer the engine sleep than intended, in ms
* RtE Count: How many robot-to-engine messages were received
* EtR Count: How many engine-to-robot messages were sent
* GtE Count: How many game-to-engine messages were received
* EtG Count: How many engine-to-game messages were sent
* Viz Count: How many vizualization messages were sent
* Battery Voltage: Current battery voltage (at one point we thought this might be helpful in debugging)
* Active Feature: The current 'active feature'
* Behavior: The current top-of-stack behavior

When results are dumped, a summary section shows extra information, including the mininum, maximum, average, and standard deviation for each of the appropriate stats.  This allows you to see, for example, the average engine tick rate, or the longest engine tick.

### Prerequisites
PerfMetric is optionally compiled into the engine.  Currently it is enabled by default for Debug builds, but disabled for Release builds.  To include it, use

```-DANKI_PERF_METRIC_ENABLED```

### Use from command line
Currently, the only interface to PerfMetric is through Victor's embedded web server.  From the command line, this will start a recording session, assuming a robot IP of 192.168.42.82:

```curl 192.168.42.82:8888/perfmetric?start```

Now some time later, one can dump the results with:

```curl 192.168.42.82:8888/perfmetric?stop&dumplogall```

Note that multiple commands can be entered at the same time, separated by ampersands.  This includes some 'wait' commands that allow you to do a recording session with one single command:

```curl 192.168.42.82:8888/perfmetric?start&waitseconds60&stop&dumplogall```

Here is the complete list of commands and what they do:
* "start" starts recording; if a recording was in progress, the buffer is reset before re-starting
* "stop" stops recording
* "dumplog" dumps the summary of results to the log
* "dumpallall" dumps the entire recorded tick buffer, along with the summary, to the log
* "dumpresponse" returns summary as HTTP response
* "dumpresponseall" returns all info as HTTP response
* "dumpfiles" writes all info to two files on the robot:  One is a formatted txt file, and the other a csv file.  These go in cache/perfMetricLogs


### Helper script
A script has been created for convenience and as an example:

```tools/perfMetric/autoPerfMetric.sh```

### Use with webots (pure simulator)
When using with webots pure simulator, use 'localhost' as your IP.  Also note that webots pure simulator does NOT sleep between engine ticks, so the output will reflect that.

### Future features
* Build into the Anim process (vic-anim)
* Record other message counts (e.g. engine-to-cloud)
* Record visual schedule mediator info
* HTML interface (start/stop buttons, and an area to see output)
* Automated performance testing

