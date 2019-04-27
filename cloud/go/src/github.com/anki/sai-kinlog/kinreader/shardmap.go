// Copyright 2016 Anki, Inc.

package kinreader

import (
	"bytes"
	"fmt"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/kinesis"
)

// shardMap tracks the state of known shards and their respective processors
type shardMap map[string]*Shard

// Update the shard map, adding unknown shards and removing deleted ones.
func (sm shardMap) update(r *Reader, shards []*kinesis.Shard) {
	for _, s := range shards {
		shardId := aws.StringValue(s.ShardId)
		if _, ok := sm[shardId]; !ok {
			sm[shardId] = &Shard{
				Shard:  s,
				reader: r,
			}
		}
		sm[shardId].mark = true
	}

	// remove dead shards from the shardmap (ie. shards that are closed and all events
	// have passed beyond the trim horizon)

	for _, s := range sm {
		if !s.mark {
			delete(sm, aws.StringValue(s.ShardId))
		}
		s.mark = false
	}
}

func (sm shardMap) parent(child *Shard) *Shard {
	parentId := child.ParentShardId
	if parentId == nil {
		return nil
	}
	return sm[*parentId]
}

func (sm shardMap) adjacentParent(child *Shard) *Shard {
	parentId := child.AdjacentParentShardId
	if parentId == nil {
		return nil
	}
	return sm[*parentId]
}

func (sm shardMap) isParentClosed(child *Shard) bool {
	if parent := sm.parent(child); parent != nil {
		return parent.isClosed()
	}
	return true
}

func (sm shardMap) isParentFinished(child *Shard, cp Checkpoints) bool {
	if parent := sm.parent(child); parent != nil {
		return parent.isFinished(cp[aws.StringValue(parent.ShardId)])
	}
	return true // no parent; must be past the trim horizon if it ever existed
}

func (sm shardMap) isAdjacentParentClosed(child *Shard) bool {
	if parent := sm.adjacentParent(child); parent != nil {
		return parent.isClosed()
	}
	return true
}

func (sm shardMap) isAdjacentParentFinished(child *Shard, cp Checkpoints) bool {
	if parent := sm.adjacentParent(child); parent != nil {
		return parent.isFinished(cp[aws.StringValue(parent.ShardId)])
	}
	return true // no parent; must be past the trim horizon if it ever existed
}

func (sm shardMap) String() string {
	var buf bytes.Buffer
	for shardId, s := range sm {
		var end string
		if s.SequenceNumberRange != nil {
			end = aws.StringValue(s.SequenceNumberRange.EndingSequenceNumber)
		}
		fmt.Fprintf(&buf, "%s : procRunning=%t local_finished=%t adj=%s parent=%s end=%s\n",
			shardId, s.processingRunning, s.localFinished,
			aws.StringValue(s.AdjacentParentShardId),
			aws.StringValue(s.ParentShardId),
			end)
	}
	return buf.String()
}
