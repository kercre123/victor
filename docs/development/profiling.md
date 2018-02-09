# Victor Profiling

Profiling on Victor is based on `simpleperf` [../project/victor/simpleperf/README.md]

## Preparation

Create and deploy a release build.

  ./project/victor/build-victor.sh -c Release

  ANKI_BUILD_TYPE=Release ./project/victor/scripts/deploy.sh

## Recording and reporting

HOW-simpleperf.sh recognizes the following environment variables, with default values shown:

  ANKI_PROFILE_PROCNAME=cozmoengined

  ANKI_PROFILE_DURATION=10 (in seconds)

  ANKI_PROFILE_FREQUENCY=4000 (in usec)

  ANKI_PROFILE_SYMBOLCACHE=symbol_cache

Default values may be overridden as needed:

  # ANKI_PROFILE_PROCNAME=victor_animator bash HOW-inferno.sh

Current Victor processes are:
* cloud_process
* cozmoengined
* robot_supervisor
* victor_animator

Additional arguments can be passed to the report generation:

  # ./HOW-simpleperf.sh --sort tid

  # ./HOW-simpleperf.sh --tids 1734

## Additional Reporting

Once HOW-simpleperf.sh has run and the initial report generated further reports can be generated. These, HOW-inferno.sh, HOW-annotate.sh, HOW-reportchart.sh, HOW-reportsamples.sh, also recognise the same environment variables.

### HOW-simpleperf.sh [../project/victor/simpleperf/HOW-simpleperf.sh]

(images/HOW-simpleperf.png)

### HOW-inferno.sh [../project/victor/simpleperf/HOW-inferno.sh]

Flame graph

(images/HOW-annotate.png)

### HOW-annotate.sh [../project/victor/simpleperf/HOW-annotate.sh]

Annotated source code.

(images/HOW-annotate.png)

### HOW-reportchart.sh [../project/victor/simpleperf/HOW-reportchart.sh]

Combine pie chart, samples and flame graph views.

(images/HOW-reportchart1.png)
(images/HOW-reportchart2.png)
(images/HOW-reportchart3.png)

### HOW-reportsamples.sh [../project/victor/simpleperf/HOW-reportsamples.sh]

Output of samples suitable for text processing.

(images/HOW-reportsamples.png)

## Additional resources

  https://perf.wiki.kernel.org/index.php/Tutorial
  https://lxr.missinglinkelectronics.com/linux/tools/perf/Documentation/

## Implementation notes

Note: HOW-simpleperf.sh is equivalent too

  python ../lib/util/tools/simpleperf/app_profiler.py -nc -np cozmoengined -r "-e cpu-cycles:u -f 4000 --duration 10" -lib symbol_cache

  The following arguments to app_profiler.py are not applicable to our environment:
    -p, --app, Profile an Android app, given the package name. Like -p com.example.android.myapp.
    --apk, When profiling an Android app, we need the apk file to recompile the app on Android version <= M.
    -a, --activity, When profiling an Android app, start an activity before profiling. It restarts the app if the app is already running.
    -t, --test, When profiling an Android app, start an instrumentation test before profiling. It restarts the app if the app is already running.
    --arch, Select which arch the app is running on, possible values are: arm, arm64, x86, x86_64. If not set, the script will try to detect it.
    --disable_adb_root, Force adb to run in non root mode
