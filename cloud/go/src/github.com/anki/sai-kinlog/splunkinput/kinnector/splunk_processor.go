// Copyright 2016 Anki, Inc.

package kinnector

import (
	"io"
	"log"

	"github.com/anki/sai-kinlog/kinreader"
	"github.com/anki/sai-kinlog/logsender/logrecord"
	"github.com/anki/sai-splunk/modinput"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/kinesis"
)

// SplunkProcessor implements the kinreader.ShardProcessor interface.
//
// It receives compressed log data from Kinesis and sends it to Splunk for indexing.
type SplunkProcessor struct {
	Input       *modinput.Input
	EventWriter modinput.EventWriter
	Logger      *log.Logger
}

// ProcessRecords decodes the supplied Kinesis records and streams the event
// data to Splunk.
func (sp *SplunkProcessor) ProcessRecords(shard *kinreader.Shard, records []*kinesis.Record) error {
	shardId := aws.StringValue(shard.ShardId)
	for _, rec := range records {
		sequenceNumber := aws.StringValue(rec.SequenceNumber)
		dec, err := logrecord.NewDecoder(rec.Data)
		if err != nil {
			sp.Logger.Printf("Error decoding record shard_id=%s sequence_number=%s error=%v",
				shardId, sequenceNumber, err)
			continue
		}
		md := dec.Metadata()
		if !md.IsUnbroken {
			// can stream all the events immediately to Splunk
			if err := sp.streamData(dec, sequenceNumber, shardId); err != nil {
				return err
			}
		} else {
			// else we have to check that the events are sequential to those already received
			panic("Processor for unbroken streams not yet implemented")
		}
	}

	// Checkpoint after the block of records has been processed to avoid sending hundreds
	// of checkpoints/second to the server
	if len(records) > 0 {
		last := records[len(records)-1]
		if err := shard.Checkpoint(aws.StringValue(last.SequenceNumber)); err != nil {
			return err
		}
	}

	return nil
}

// Decompress and stream events to Splunk
func (sp *SplunkProcessor) streamData(dec *logrecord.Decoder, recordId, shardId string) error {
	var data logrecord.Data
	md := dec.Metadata()
	event := modinput.Event{
		Source:     md.Source,
		Sourcetype: md.Sourcetype,
		Host:       md.Host,
		Index:      md.Index,
		IsUnbroken: md.IsUnbroken,
	}
	zeroTime := event.Time
	for {
		if err := dec.Decode(&data); err == io.EOF {
			return nil
		} else if err != nil {
			return err
		}
		event.IsDone = data.IsDone
		event.Data = data.Data
		if data.Time != nil {
			event.Time = *data.Time
		} else {
			event.Time = zeroTime
		}
		sp.EventWriter.Write(&event)
	}
}
