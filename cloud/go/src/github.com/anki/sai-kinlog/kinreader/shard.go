// Copyright 2016 Anki, Inc.

package kinreader

import (
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/kinesis"
)

// Shard wraps a Kinesis Shard and provides checkpointing facilities.
type Shard struct {
	*kinesis.Shard
	reader            *Reader
	processingRunning bool
	localFinished     bool // true if this process has read all data, whether or not a CP has yet been set
	defaultToLatest   bool
	mark              bool // for mark/sweep collection of dead shards
}

// Checkpoint sets the current checkpoint on a shard, preventing  re-reads of
// prior records on future runs.
//
// To avoid overloading the backing database store, its recommended to call
// Checkpoint from a ShardProcessor after processing a group of Records, or
// after a time limit has been reached (eg. every 250ms).
func (s *Shard) Checkpoint(seqnum string) error {
	if seqnum == "" {
		return nil
	}
	return s.reader.checkpointer.SetCheckpoint(Checkpoint{
		ShardId:    aws.StringValue(s.ShardId),
		Time:       time.Now(),
		Checkpoint: seqnum,
	})
}

// mark the shard as finished in the checkpointer
func (s *Shard) checkpointClosed() error {
	if end := s.SequenceNumberRange.EndingSequenceNumber; end != nil {
		return s.Checkpoint(*end)
	}
	return nil
}

func (s *Shard) isClosed() bool {
	return s.SequenceNumberRange != nil && s.SequenceNumberRange.EndingSequenceNumber != nil
}

func (s *Shard) isFinalSequenceNumber(seqnum string) bool {
	if s.SequenceNumberRange.EndingSequenceNumber == nil {
		return false
	}
	return aws.StringValue(s.SequenceNumberRange.EndingSequenceNumber) == seqnum
}

// isFinished returns true if the shard is both closed and either the last shard iterator
// was reached during a local run, or the ending sequence number is equal to the checkpoint number
//
// Returns false if cp is nil
func (s *Shard) isFinished(cp *Checkpoint) bool {
	var isfincp bool
	if cp != nil {
		isfincp = s.isFinalSequenceNumber(cp.Checkpoint)
	}
	return s.localFinished || (cp != nil && s.isClosed() && isfincp)
}
