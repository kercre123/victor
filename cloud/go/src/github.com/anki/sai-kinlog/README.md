AWS Kinesis Log Stream for Go
=============================

kinlog provides a number of packages that allow for log data to be streamed
from applications to Kinesis for downstream retrieval and loading into a
Splunk Indexer.

Kinesis is an Amazon Web Services services that provides a low-latency, highly
scalable buffered data stream.  By default it allows data to be buffered for
24 hours; this allows log producers to continuously stream data without risk
of data loss, even if Splunk is down for maintenance or becomes otherwise
inaccessible.

It also allow for data to be read by multiple consumers, so could also be used
to concurrently stream data to S3 for archiving.


## Packages

This repo provides several packages:

* **logsender** - Encodes, buffers and sends log data into a Kinesis stream
  * **kinrecord** - Encoder and Decodes log data to/from a Kinesis Record
* **kinreader** - A high level interface to read records from Kinesis
* **splunkinput** - A Splunk Modular Input that reads log data from a Kinesis stream


## Logsender Usage

The logsender package allows Go applications to send their log data directly
into a Kinesis stream.  It provides a standard io.Writer that can be handed
to the log package, or any other log generator.

The minimum configuration requirements are just the name of the log stream
and Amazon AWS credentials.  The latter may be picked up automatically
from EC2 metadata, or from `AWS_REGION`, `AWS_ACCESS_KEY_ID` and
`AWS_SECRET_KEY_ID` environment variables, or from $HOME/.aws/credentials.

A log stream consists of one or more shards.  The logsender package will
automatically ensure log data is distributed across available shards.

Streaming to Kinesis happens in the background and will not block the
application during writes.

**IMPORTANT**: Be sure to call the Stop method on the Sender prior to exiting
the program to ensure all outstanding data has been successfully flushed to
Kinesis.

See the package documentation for examples.


## Splunk Modular Input Usage ##

The provided modular input reads data sent to a Kinesis stream by one or
more logsenders and streams it into Splunk for indexing.

The application can be built by running `make splunkapp` - That will generate
a .tar.gz file that can be installed as an app into Splunk using
`splunk install app kinloader.tar.gz`.

Once installed a new input can be configured from the Splunk UI in
`Settings -> Data Inputs ->  Kinesis Log Processor`.  Data ingestion will
start as soon as the input is saved.

If AWS access credentials are not provided then the input will attempt to
collect credentials from the filesystem, environment or EC2 metadata.

At the current time only a single modular input instance per Kinesis stream
is supported (ie. it can not be allowed to run concurrently on multiple
machines for a given stream).


### Resharding ###

Kinesis allows for individual shards to be split or merged as required to
balance throughput against cost.  The modular input will automatically
complete reading of a closed shard before starting on data from its children.


### Checkpointing ###

The modular input initially starts reading from the "tip" of the Kinesis stream.
That is to say, it starts with the most recent data first and continues from
there.

As it loads data into Splunk it updates a checkpoint table to keep track
of which records it has processed from which shard, avoiding data duplicates
after a crash or restart.

The input can store these checkpoints in one of three locations:

* Splunk KV store - Splunk itself provides a strongly consistent key/value store
which can be replicated between indexers.
* DynamoDB table - If so configured, the input can attempt to create a Dynamo
DB table for each stream and store checkpoint information therein.  It
defaults to creating a table with a read/write capacity of 10 which may
need to be increased if there are a large number of shards.
* Filesystem - Splunk provides a directory on disk for the input to store
checkpoint data.  This directory is not replicated, however, so a loss of
the filesystem could result in duplicate data being read from Kinesis.

### Limitations ###

The kinreader package upon which the modular input is based does not yet
support concurrent readers - There must only be one instance of the modular
input running at a time, per stream.

## Utilties

### dumpstream

The `dumpstream` command (`logreader/cmd/dumpstream`) connects to a Kinesis
stream and reads raw records from it.  It can optionally keep track of where
it last read from by writing to a checkpoint file, else it can begin reading
from the earliest record (the "horizon") or the latest (most recent) entry
in the stream.

NOTE: If starting from the horizon, it may take a couple of minutes to see
the first record if the stream has a sparse amount of data in it:
Kinesis requires multiple calls to get shard iterators and fetch records
before reaching a part of the stream that has data.

```
$ ./dumpstream --help

Usage: dumpstream --stream-name [--checkpoint-file] [--data-format] [--filename] [--max-rate] [--default-to-horizon]

Dumps the content from a Kinesis stream

Options:
  --stream-name=""             Kinesis stream name to read from ($LOG_KINESIS_STREAM)
  --checkpoint-file=""         File to store checkpoint information.  Default to in-memory checkpointing ($CHECKPOINT_FILE)
  --data-format="json"         Which format to emit the data in - either "json" or "raw" ($DATA_FORMAT)
  --filename=""                Which file to write the output to; defaults to stdout ($TARGET_FILENAME)
  --max-rate=3                 Maximum number of requests to Kinesis to make per second (Kinesis supports 5 reads/second/shard) ($MAX-RATE)
  --default-to-horizon=false   Enable to cause data to start reading from the earliest available point in the stream, rather than the latest if no checkpoint file exists ($DEFAULT_TO_HORIZION)
```

### logdecode

The `logdecode` command (`logreader/cmd/logdecode`) reads data dumped by the
`dumpstream` command and decompresses & decodes the log data contaiend therein.

It can send the log data to the console, or can split it up into different files
by source, sourcetype, etc.

If can also either just emit the log data, or JSON records that also contain each
event's metadata.

To view a realtime tail of the logs entering a Kinesis stream, the output
of `dumpstream` can be piped into `logdecode`:

`./dumpstream  --stream-name splunk_logs_development 2>/dev/null | ./logdecode`


To dump all of the logs in a stream to individual files:

```
./dumpstream --stream-name splunk_logs_development --default-to-horizon \
	| ./logdecode --target-filepath 'output/%source%/%source%-%host%.log'"
```

(this will, of course, take a while to run to dump all 24 hours of data in the stream)

```
$ ./logdecode --help

Usage: logdecode [--target-filepath] [--indent-json] [--format=<raw|json>]

Reads and converts log data dumped by dumpstream

Options:
  -f, --target-filepath=""   Filepath to write to.  Will create any directories as needed and may include %source%, %sourcetype%, %host% and %index% variables which will be expanded. eg. "logdata/%index/%source%-%host%.log".  Defaults to stdout
  --format="raw"             'raw' or 'json' - Output as raw log data, or as JSON including metadata
  --indent-json=false        Enables JSON indenting for humanish readable output
```

### log2kinesis

The `log2kinesis` command (`logsender/cmd/log2kinesis`) sends events
to a Kinesis stream.  It has a single sub-command called `send`

It can be useful in conjunction with bash scripts to send data to the log.

It can send the entire input as a single event, send each line as a separate
event, or apply a regular expression to detect the start of each event for
multiline events (eg. a timestamp).

By default it will split each line into a separate event.

```
echo "Test log entry" | log2kinesis send --stream-name test-stream --source=test-script
```

It can also execute commands and capture their stdout/stderr to be used a log stream.

Eg. to send the entire output of the df command as a single event with no line
splitting:

```
log2kinesis send \
    --stream-name test-stream \
    --source=system-disk-space \
    --split-mode none \
    df -- -H
```

**NOTE**: The `--` is needed after the `df` command in the above line so the remaining
options are not confused as options for the `send-event` command, but are instead passed
on to the `df` command.

```
$ log2kinesis send --help

Usage: log2kinesis send --stream-name --source [--sourcetype] [--host] [--index] [--timestamp] [--capture] [--split-mode] [--split-regex] [--max-lines] [CMD [ARGS...]]

Read events from stdin and send to Kinesis

Arguments:
  CMD=""       The command to execute
  ARGS=[]      The command arguments

Options:
  --stream-name="testsplit"                 Kinesis stream name to send to.  Set to "testsplit" to print split events to stdout and validate --split-mode settings ($LOG_KINESIS_STREAM)
  --source=""                               Source name to assign to log data ($LOG_SOURCE)
  --sourcetype="generic"                    Sourcetype to assign to log data ($LOG_SOURCETYPE)
  --host="godev"                            Host name to assign to log data ($LOG_HOST)
  --index=""                                Index to send log data to ($LOG_INDEX)
  --timestamp="2006-01-02T15:04:05Z07:00"   Prefix events with specified timestamp format; set to empty string to omit timestamp. ($TIMESTAMP_FMT)
  --capture=stdout                          File descriptor to capture for executed commands.  Either stdout or stderr ($CMD_CAPTURE)
  --split-mode=newline                      Set event splitting method.  If "none" then the entire input will be treated as a single event.  If "newline" then each line will be treated as a single event.  If "regex" then events will b)
  --split-regex="^\\d{4}-\\d{2}-\\d{2}"     Sets the regular expression to use to split events when split-mode is set to regex ($SPLIT_REGEX)
  --max-lines=1000                          The maximum number of lines to buffer for a single event when used with split-mode=(none|regex) ($MAX_EVENT_LINES)
```
