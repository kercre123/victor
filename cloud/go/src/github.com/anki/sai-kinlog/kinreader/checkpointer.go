// Copyright 2016 Anki, Inc.

package kinreader

import "time"

// Checkpoint holds the last record id processed for a single Kinesis shard.
type Checkpoint struct {
	ShardId    string    `json:"shard_id"`
	Time       time.Time `json:"timestamp"`
	Checkpoint string    `json:"checkpoint"`
}

// Checkpoints holds the current set of known checkpoints
type Checkpoints map[string]*Checkpoint

// A Checkpointer is capable of setting and retrieving shard checkpoints.
//
// Types implementing this interface must be prepared to handle requests
// from concurrent goroutines.
type Checkpointer interface {
	SetCheckpoint(cp Checkpoint) error
	AllCheckpoints() (Checkpoints, error)
}
