// Copyright 2016 Anki, Inc.

package kinreader

import (
	"reflect"
	"sort"
	"testing"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/kinesis"
)

func mkOpenShard(id string) *Shard {
	return &Shard{
		Shard: &kinesis.Shard{
			ShardId:             aws.String(id),
			SequenceNumberRange: &kinesis.SequenceNumberRange{},
		},
	}
}

func mkClosedShard(id, lastSeq string) *Shard {
	return &Shard{
		Shard: &kinesis.Shard{
			ShardId: aws.String(id),
			SequenceNumberRange: &kinesis.SequenceNumberRange{
				EndingSequenceNumber: aws.String(lastSeq),
			},
		},
	}
}

func TestShardMapUpdate(t *testing.T) {
	r := &Reader{}
	sm := shardMap{
		"shard-001": mkOpenShard("shard-001"),
		"shard-002": mkOpenShard("shard-002"),
		"shard-003": mkOpenShard("shard-003"),
		"shard-004": mkOpenShard("shard-004"),
	}

	// create a list that removes 2 and 3 and adds 7
	newShardList := []*kinesis.Shard{
		&kinesis.Shard{ShardId: aws.String("shard-001")},
		&kinesis.Shard{ShardId: aws.String("shard-004")},
		&kinesis.Shard{ShardId: aws.String("shard-005")},
		&kinesis.Shard{ShardId: aws.String("shard-007")},
	}

	sm.update(r, newShardList)
	expected := []string{"shard-001", "shard-004", "shard-005", "shard-007"}
	actual := []string{}
	for id, shard := range sm {
		actual = append(actual, id)
		if id != aws.StringValue(shard.ShardId) {
			t.Errorf("shard id %s did not map to correct shard instance %#v", id, shard)
		}
		if shard.mark {
			t.Errorf("shard %s was left with mark=true", id)
		}
	}
	sort.Strings(actual)
	if !reflect.DeepEqual(actual, expected) {
		t.Fatalf("shard map had incorrect ids expected=%v actual=%v", expected, actual)
	}

	if sm["shard-007"].reader != r {
		t.Error("Shard did not have reader attached")
	}
}

var smParentTests = []struct {
	name          string
	testShard     string
	parentClosed1 bool // cp of parent before end
	parentClosed2 bool // cp of parent at end
	adjClosed1    bool // cp of adjacent before end
	adjClosed2    bool // cp of adjacent at end
}{
	{"no-parents", "shard-005", true, true, true, true},
	{"merged", "shard-003", false, true, false, true},
	{"split", "shard-004", false, true, true, true}, // a split shard doesn't have an adjancent parent, so always closed
}

func TestShardMapParentFinished(t *testing.T) {
	sm := shardMap{
		"shard-001": mkClosedShard("shard-001", "100"),
		"shard-002": mkClosedShard("shard-002", "200"),
		"shard-003": mkOpenShard("shard-003"), // parent and ajacent (merged shard)
		"shard-004": mkOpenShard("shard-004"), // parent only (split shard)
		"shard-005": mkOpenShard("shard-006"), // no parents
	}
	sm["shard-003"].Shard.ParentShardId = aws.String("shard-001")
	sm["shard-003"].Shard.AdjacentParentShardId = aws.String("shard-002")
	sm["shard-004"].Shard.ParentShardId = aws.String("shard-001")

	for _, test := range smParentTests {
		shard := sm[test.testShard]
		cp := Checkpoints{} // no checkpoints
		if result := sm.isParentFinished(shard, cp); result != test.parentClosed1 {
			t.Errorf("test=%q expected=%t actual=%t", test.name, test.parentClosed1, result)
		}
		if result := sm.isAdjacentParentFinished(shard, cp); result != test.adjClosed1 {
			t.Errorf("test=%q expected=%t actual=%t", test.name, test.adjClosed1, result)
		}

		cp = Checkpoints{
			"shard-001": &Checkpoint{Checkpoint: "99"}, // parent not finished
			"shard-002": &Checkpoint{Checkpoint: "99"}, // adj not finished
		}

		// should test the same as if no checkpoints were set
		if result := sm.isParentFinished(shard, cp); result != test.parentClosed1 {
			t.Fatalf("test=%q expected=%t actual=%t", test.name, test.parentClosed1, result)
		}
		if result := sm.isAdjacentParentFinished(shard, cp); result != test.adjClosed1 {
			t.Errorf("test=%q expected=%t actual=%t", test.name, test.adjClosed1, result)
		}

		cp = Checkpoints{
			"shard-001": &Checkpoint{Checkpoint: "100"}, // parent finished
			"shard-002": &Checkpoint{Checkpoint: "99"},  // adj not finished
		}

		if result := sm.isParentFinished(shard, cp); result != test.parentClosed2 {
			t.Errorf("test=%q expected=%t actual=%t", test.name, test.parentClosed2, result)
		}
		if result := sm.isAdjacentParentFinished(shard, cp); result != test.adjClosed1 {
			t.Errorf("test=%q expected=%t actual=%t", test.name, test.adjClosed1, result)
		}

		cp = Checkpoints{
			"shard-001": &Checkpoint{Checkpoint: "100"}, // parent finished
			"shard-002": &Checkpoint{Checkpoint: "200"}, // adj finished
		}

		if result := sm.isParentFinished(shard, cp); result != test.parentClosed2 {
			t.Errorf("test=%q expected=%t actual=%t", test.name, test.parentClosed2, result)
		}
		if result := sm.isAdjacentParentFinished(shard, cp); result != test.adjClosed2 {
			t.Errorf("test=%q expected=%t actual=%t", test.name, test.adjClosed2, result)
		}
	}
}
