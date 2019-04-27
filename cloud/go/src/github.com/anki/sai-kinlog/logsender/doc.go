// Copyright 2016 Anki, Inc.

/*
Package logsender accepts textual log data and sends it to a Kinesis stream.

It can send either a stream of individual (pre-broken) events using an
EventWriter, or a stream of unbroken log data using a StreamWriter.

For pre-broken events, log data is distributed amongst all available shards by
generating random Kinesis partition keys to make full use of available
capacity.

Each block of log data has a metadata key associated with it, which describes
the host, source, sourcetype and target index for the log data.

For unbroken data, log data for a given metadata key is sent to the same shard
so that it will be processed by a single reader and will be reconstructed
in strict order, so that Splunk may correct parse and split out individual log
entries.

A Sender orchestrates the collection and transmission of log data to Kinesis.
It will buffer log data in memory and dispatch to Kinesis in the background.

By default data is flushed to Kinesis every second, or whenever a buffer is
full; there's no need to flush manually.

Calling Stop on a Sender before the program exits ensures that all outstanding
data is successfully flushed prior to program exit.

Call NewEventWriter on a Sender to create a writer for log data that has the
same metadata.

By default a maximum of three Kinesis PutRecords operations may be in flight
at any given time and data will continued to be buffered while waiting, up
to the limit of a Kinesis record (1MB of compressed log data).

If the maximum number of requests are in flight and the pending buffer is full,
then calls to Write will return an ErrQueueFull error.

Under normal circumstances, Kinesis is a low latency data sink and has no
problem keeping up with data input as long as a sufficent number of shards
have been provisioned.

A Sender tests its connection to Kinesis on creation and will return an error
if it has been mis-configured.  Transient errors while sending to Kinesis are
automatically retried, but if a hard error occurs (eg. because Kinesis itself
is down, or the application no longer has access to the stream) then the
sender will execute its configured ErrorHandler which may instruct the sender
to retry, discard the data or may deliberately crash the program.

By default if a hard error occurs (and the default retries have failed) then
the sender will panic.

Example

An example to send log data to a Kinesis stream called "mystream":

	import (
		"log"
		"github.com/anki/sai-kinlog/logsender"
		"github.com/anki/sai-kinlog/logsender/logrecord"
	)

	func main() {
		sender, err := logsender.New("mystream")
		if  err != nil {
			log.Fatal("Failed to create log sender: ", err)
		}
		defer sender.Stop()

		md := logrecord.Metadata{
			Source: "myappname",
			Sourcetype: "mysourcetype",
		}
		w := sender.NewEventWriter(md, false)
		log.SetOutput(w)

		log.Println("It works!")
	}

*/
package logsender
