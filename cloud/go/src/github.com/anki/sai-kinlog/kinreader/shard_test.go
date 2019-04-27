// Copyright 2016 Anki, Inc.

package kinreader

import (
	"testing"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/kinesis"
)

func TestShardSeqNumTest(t *testing.T) {
	s := &Shard{
		Shard: &kinesis.Shard{
			SequenceNumberRange: &kinesis.SequenceNumberRange{
				EndingSequenceNumber: aws.String("1234"),
			},
		},
	}
	if s.isFinalSequenceNumber("1233") {
		t.Error("returned true, should be false")
	}
	if !s.isFinalSequenceNumber("1234") {
		t.Error("returned false, should be true")
	}

	s.Shard.SequenceNumberRange.EndingSequenceNumber = nil
	if s.isFinalSequenceNumber("1233") {
		t.Error("returned true, should be false")
	}
}

var isFinishedTests = []struct {
	name             string
	closedShard      bool
	localFinishedSet bool
	cpSet            bool
	cpBefore         bool
	expected         bool
}{
	{"not-closed", false, false, true, false, false},               // can never be finished if not closed
	{"no-checkpoint", true, false, false, false, false},            // can't be finished if no checkpoint set (never read)
	{"before-end", true, false, true, true, false},                 // closed, but checkpoint before end == not finished
	{"before-end-local-done", true, true, true, true, true},        // closed, checkpoint before end, but locally finished
	{"before-end-local-done-no-cp", true, true, false, true, true}, // closed, checkpoint before end, but locally finished
	{"all-done", true, false, true, false, true},                   // closed and all data read == finished
}

func TestShardIsFinished(t *testing.T) {
	for _, test := range isFinishedTests {
		s := &Shard{Shard: &kinesis.Shard{
			ShardId:             aws.String("test"),
			SequenceNumberRange: &kinesis.SequenceNumberRange{}}}
		if test.closedShard {
			s.Shard.SequenceNumberRange.EndingSequenceNumber = aws.String("100")
		}
		var cp *Checkpoint
		if test.cpSet {
			if test.cpBefore {
				cp = &Checkpoint{Checkpoint: "99"}
			} else {
				cp = &Checkpoint{Checkpoint: "100"}
			}
		}
		if test.localFinishedSet {
			s.localFinished = true
		}
		result := s.isFinished(cp)
		if result != test.expected {
			t.Errorf("name=%s expected=%t actual=%t", test.name, test.expected, result)
		}
	}
}
