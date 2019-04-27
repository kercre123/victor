// Copyright 2016 Anki, Inc.

/*
Package kinreader provides high-level read access to a Kinesis stream.

The Reader takes care of iterating over shards, handling splits, merges and
closed Shards and sends batches of records to a registered ShardProcessor.

The ShardProcessor can keep track of which records it has successfully
processed by updating a CheckPointer:  Once checkpointed, the Reader will
not send the same record again to the processor, preventing duplicates.

Unlike the official Kinesis Client, this library does not yet support
client lease acquisition of shards.  That implies that there is no support for
concurrent access to the checkpoint store, and so only a single Reader must
be processing a stream at a time.

A future update will add support for work distribution.

Example

Connects to a Kinesis stream and prints the metadata for incoming events,
checkpointing after each group.

    package main

    import (
            "fmt"
            "log"

            "github.com/anki/sai-kinlog/kinreader"
            "github.com/aws/aws-sdk-go/aws"
            "github.com/aws/aws-sdk-go/service/kinesis"
    )

    func processor(shard *kinreader.Shard, records []*kinesis.Record) error {
            var last string
            for _, rec := range records {
                    fmt.Printf("Got record with shard=%q sequence_id=%q partition_key=%q\n",
                            aws.StringValue(shard.ShardId), aws.StringValue(rec.SequenceNumber),
                            aws.StringValue(rec.PartitionKey))
                    last = aws.StringValue(rec.SequenceNumber)
            }
            return shard.Checkpoint(last)
    }

    func main() {
            cp, _ := kinreader.NewFileCheckpointer("/tmp/cp.json")
            p := kinreader.ShardProcessorFunc(processor)
            r, err := kinreader.New(
                    "my_kinesis_stream",
                    kinreader.WithCheckpointer(cp),
                    kinreader.WithProcessor(p))
            if err != nil {
                    log.Fatal(err)
            }
            log.Fatal(r.Run())
    }
*/
package kinreader
