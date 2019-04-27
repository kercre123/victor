// Copyright 2016 Anki, Inc.

package kinreader

import (
	"fmt"
	"log"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/kinesis"
)

// A ShardProcessor is used by a Reader to process records read from
// a Kinesis shard.
//
// ProcessRecords is called in its own goroutine; it should iterate over
// the supplied records and call shard.Checkpoint periodically to ensure
// records aren't supplied twice.
//
// Calling Checkpoint after every single record may put an excessive load
// on the backing store if processing lots of small records at high frequency.
// Instead checkpoint in batches, or by time and before returning from the call.
type ShardProcessor interface {
	ProcessRecords(shard *Shard, records []*kinesis.Record) error
}

// ShardProcessorFunc is an adapter function to make a regular function comply
// with the ShardProcessor interface.
type ShardProcessorFunc func(shard *Shard, records []*kinesis.Record) error

// ProcessRecords calls f(shard, records)
func (f ShardProcessorFunc) ProcessRecords(shard *Shard, records []*kinesis.Record) error {
	return f(shard, records)
}

// ShardLogger is a processor that logs the metadata of all incoming items from
// a shard to a defined logger.  Generally only useful for debugging.
type ShardLogger struct {
	Log *log.Logger
}

// ProcessRecords sends the supplied records to the ShardLogger's Log.
func (sl *ShardLogger) ProcessRecords(shard *Shard, records []*kinesis.Record) error {
	for _, r := range records {
		msg := fmt.Sprintf("shard=%q sequence_id=%q partition_key=%q arrival_time=%s data_len=%d",
			aws.StringValue(shard.ShardId), aws.StringValue(r.SequenceNumber),
			aws.StringValue(r.PartitionKey), r.ApproximateArrivalTimestamp, len(r.Data))
		if sl.Log == nil {
			log.Println(msg)
		} else {
			sl.Log.Println(msg)
		}
	}
	return nil
}
