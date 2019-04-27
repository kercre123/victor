package kinreader

import (
	"errors"
	"fmt"
	"reflect"
	"sort"
	"strconv"
	"sync"
	"testing"
	"time"
)
import (
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/kinesis"
	"github.com/kr/pretty"
)

type mockGetShardIterator struct {
	m               sync.Mutex
	returnIterators map[string][]string // map shard ids to []iterators
	callCount       map[string]int      // map shard ids to call count
}

func (m *mockGetShardIterator) init() {
	if m.callCount == nil {
		m.callCount = make(map[string]int)
	}
}

func (m *mockGetShardIterator) GetShardIterator(input *kinesis.GetShardIteratorInput) (*kinesis.GetShardIteratorOutput, error) {
	m.m.Lock()
	defer m.m.Unlock()
	m.init()
	shardId := aws.StringValue(input.ShardId)
	it, ok := m.returnIterators[shardId]
	if !ok {
		return nil, fmt.Errorf("getShardIterator called for unknown shard %s", aws.StringValue(input.ShardId))
	}
	m.returnIterators[shardId] = it[1:]
	m.callCount[shardId]++
	return &kinesis.GetShardIteratorOutput{ShardIterator: aws.String(it[0])}, nil
}

type mockDescribeStream struct {
	input       []*kinesis.DescribeStreamInput
	responses   []*kinesis.DescribeStreamOutput
	repeatFirst bool
}

func (ds *mockDescribeStream) DescribeStream(input *kinesis.DescribeStreamInput) (*kinesis.DescribeStreamOutput, error) {
	ds.input = append(ds.input, input)
	if len(ds.responses) == 0 {
		return nil, errors.New("No DescribeStream responses remaining")
	}
	resp := ds.responses[0]
	if !ds.repeatFirst {
		ds.responses = ds.responses[1:]
	}
	return resp, nil
}

type mockGetRecords struct {
	m                 sync.Mutex
	recordsByIterator map[int64][]*kinesis.Record
}

func (m *mockGetRecords) init() {
	if m.recordsByIterator == nil {
		m.recordsByIterator = make(map[int64][]*kinesis.Record)
	}
}

func (m *mockGetRecords) GetRecords(input *kinesis.GetRecordsInput) (*kinesis.GetRecordsOutput, error) {
	m.m.Lock()
	defer m.m.Unlock()
	m.init()
	it, _ := strconv.ParseInt(aws.StringValue(input.ShardIterator), 10, 64)
	pretty.Println(it, m.recordsByIterator)
	recs, ok := m.recordsByIterator[it]
	if !ok {
		return nil, fmt.Errorf("No GetRecords entry for iterator %v", it)
	}
	delete(m.recordsByIterator, it) // ensure GetRecords isn't called twice for same iterator
	if _, ok := m.recordsByIterator[it+1]; ok {
		return &kinesis.GetRecordsOutput{NextShardIterator: aws.String(strconv.FormatInt(it+1, 10)), Records: recs}, nil
	}
	// end of shard
	return &kinesis.GetRecordsOutput{Records: recs}, nil
}

type mockKinesis struct {
	*mockDescribeStream
	*mockGetShardIterator
	*mockGetRecords
}

type mockShardProcessor struct {
	m           sync.Mutex
	recsByShard map[string][]*kinesis.Record
}

func (m *mockShardProcessor) init() {
	if m.recsByShard == nil {
		m.recsByShard = make(map[string][]*kinesis.Record)
	}
}

func (m *mockShardProcessor) ProcessRecords(shard *Shard, records []*kinesis.Record) error {
	m.m.Lock()
	defer m.m.Unlock()
	m.init()
	shardId := aws.StringValue(shard.ShardId)
	m.recsByShard[shardId] = append(m.recsByShard[shardId], records...)
	return nil
}

func (m *mockShardProcessor) sequenceIdsForShard(shardId string) (ids []string) {
	for _, rec := range m.recsByShard[shardId] {
		ids = append(ids, aws.StringValue(rec.SequenceNumber))
	}
	return ids
}

func mkRecord(partitionKey, seqNum, data string) *kinesis.Record {
	return &kinesis.Record{
		PartitionKey:   aws.String(partitionKey),
		SequenceNumber: aws.String(seqNum),
		Data:           []byte(data),
	}
}

func withShortTimer() Config {
	return func(r *Reader) error {
		r.newTimer = func(d time.Duration) *time.Timer {
			return time.NewTimer(5 * time.Millisecond)
		}
		return nil
	}
}

// Read 2 blocks of records from a shard and then check that it exits cleanly when the shard is finished
// This tests that shard iterator are followed in order, records are sent to the processor correctly
// and the function recognizes that the end of the shard has been reached and exits.
func TestProcessShardToFinished(t *testing.T) {
	records1 := []*kinesis.Record{mkRecord("p1", "001", "rec1"), mkRecord("p1", "002", "rec2")}
	records2 := []*kinesis.Record{mkRecord("p1", "003", "rec3")}

	recordSet := map[int64][]*kinesis.Record{
		100: records1,
		101: records2,
	}
	k := &mockKinesis{
		mockGetRecords: &mockGetRecords{
			recordsByIterator: recordSet,
		},
		mockGetShardIterator: &mockGetShardIterator{
			returnIterators: map[string][]string{
				"shard-001": []string{"100"},
			},
		},
	}

	mp := &mockShardProcessor{}

	reader, _ := New("test_stream", WithKinesis(k), WithProcessor(mp), withShortTimer())

	shard := &Shard{
		Shard: &kinesis.Shard{
			ShardId: aws.String("shard-001"),
		},
	}

	go reader.processShard(shard, "")

	var status shardStatus
	select {
	case <-time.After(time.Second):
		t.Fatal("Timed out")
	case status = <-reader.shardStatus:
	}

	if !status.isFinished {
		t.Fatal("shard was not set to isFinished")
	}

	// should have received all three records, in order
	expectedIds := []string{"001", "002", "003"}
	actual := mp.sequenceIdsForShard("shard-001")

	if !reflect.DeepEqual(expectedIds, actual) {
		t.Errorf("Did not process expected record sequence ids expected=%#v actual=%#v",
			expectedIds, actual)
	}
}

// Test that closing stopChan causing the processor to exit cleanly
func TestProcessShardStop(t *testing.T) {
	records1 := []*kinesis.Record{mkRecord("p1", "001", "rec1"), mkRecord("p1", "002", "rec2")}
	records2 := []*kinesis.Record{mkRecord("p1", "003", "rec3")}

	recordSet := map[int64][]*kinesis.Record{
		100: records1,
		101: records2,
	}
	k := &mockKinesis{
		mockGetRecords: &mockGetRecords{
			recordsByIterator: recordSet,
		},
		mockGetShardIterator: &mockGetShardIterator{
			returnIterators: map[string][]string{
				"shard-001": []string{"100"},
			},
		},
	}

	mp := &mockShardProcessor{}

	reader, _ := New("test_stream", WithKinesis(k), WithProcessor(mp), withShortTimer())
	shard := &Shard{
		Shard: &kinesis.Shard{
			ShardId: aws.String("shard-001"),
		},
	}

	// sets r.stop = true and closes r.stopShards
	reader.Stop()

	go reader.processShard(shard, "")

	var status shardStatus
	select {
	case <-time.After(time.Second):
		t.Fatal("Timed out")
	case status = <-reader.shardStatus:
	}

	if status.isFinished {
		t.Fatal("shard set to isFinished; should not have completed")
	}
	if !status.isStopped {
		t.Fatal("shard was not set to isStopped")
	}

	// expect it to have processes the first record set and then have exited
	expectedIds := []string{"001", "002"}
	actual := mp.sequenceIdsForShard("shard-001")

	if !reflect.DeepEqual(expectedIds, actual) {
		t.Errorf("Did not process expected record sequence ids expected=%#v actual=%#v",
			expectedIds, actual)
	}

}

// Start the loop processing a single shard with no checkpoints at start
func TestDispatcherSingleShardOK(t *testing.T) {
	// have processor block until released
	records1 := []*kinesis.Record{mkRecord("p1", "001", "rec1"), mkRecord("p1", "002", "rec2")}
	records2 := []*kinesis.Record{mkRecord("p1", "003", "rec3")}

	recordSet := map[int64][]*kinesis.Record{
		100: records1,
		101: records2,
	}
	k := &mockKinesis{
		mockDescribeStream: &mockDescribeStream{
			repeatFirst: true,
			responses: []*kinesis.DescribeStreamOutput{
				{
					StreamDescription: &kinesis.StreamDescription{
						StreamStatus: aws.String("ACTIVE"),
						Shards: []*kinesis.Shard{
							{ShardId: aws.String("shard-001")},
						},
					},
				},
			},
		},
		mockGetRecords: &mockGetRecords{
			recordsByIterator: recordSet,
		},
		mockGetShardIterator: &mockGetShardIterator{
			returnIterators: map[string][]string{
				"shard-001": []string{"100"},
			},
		},
	}

	gotRec := make(chan *kinesis.Record, 10)
	releaseBlock := make(chan struct{})
	mp := ShardProcessorFunc(func(shard *Shard, records []*kinesis.Record) error {
		gotRec <- records[0]
		<-releaseBlock
		return nil
	})

	reader, _ := New("test_stream", WithKinesis(k), WithProcessor(mp), withShortTimer())
	shard := &Shard{
		Shard: &kinesis.Shard{
			ShardId: aws.String("shard-001"),
		},
	}

	readerDone := make(chan error)
	go func() { readerDone <- reader.Run() }()

	// read a record from the fake record processor
	var rec *kinesis.Record
	select {
	case rec = <-gotRec:
	case <-time.After(time.Second):
		t.Fatal("Timeout waiting for record")
	}

	reader.Stop()

	// release the processor so the shard can stop
	close(releaseBlock)

	select {
	case <-time.After(time.Second):
		t.Fatal("Timeout waiting for Stop")
	case err := <-readerDone:
		if err != nil {
			t.Error("Unexpected error from Run", err)
		}
	}

	// check that we received the first record correctly
	if num := aws.StringValue(rec.SequenceNumber); num != "001" {
		t.Errorf("Incorrect sequence id: %s", num)
	}

	// check that the shard processor was stopped
	if shard.processingRunning {
		t.Fatal("Shard processing was not stopped")
	}
}

// Test that the dispatcher errors out if the stream isn't active or updating
func TestDispatcherInactiveStream(t *testing.T) {
}

// Test that shard splitting causes the reader to complete the existing shard
// before starting on the new one
func TestDispatcherSplitShard(t *testing.T) {
	// create stream with
}

func TestDispatcherMergeShards(t *testing.T) {
}

var processParentsTests = []struct {
	name             string
	parentCheckpoint string
	adjCheckpoint    string
	parentRunning    bool
	expectedStarted  int // how many new processors should be started
	expectedRunning  []string
}{
	// no checkpoints and no processors running; both parents
	// and 004 orphan should start processing data
	{"no-cp", "", "", false, 3, []string{"shard-001", "shard-002", "shard-004"}},

	// Same, except shard-001 is already marked as running, so
	// shouldn't be started again
	{"no-cp-run", "", "", true, 2, []string{"shard-001", "shard-002", "shard-004"}},

	// with checkpoints; shard-001 already completed, don't start
	// shard-002 not completed.. don't start the child 003
	{"with-cp", "100", "199", false, 2, []string{"shard-002", "shard-004"}},

	// both parents completed, start 003 this time
	{"parents-done", "100", "200", false, 2, []string{"shard-003", "shard-004"}},
}

func TestStartShardProcessorUnfinishedParents(t *testing.T) {
	k := &mockKinesis{
		mockGetRecords:       new(mockGetRecords),
		mockGetShardIterator: new(mockGetShardIterator),
	}

	mp := &mockShardProcessor{}

	reader, _ := New("test_stream", WithKinesis(k), WithProcessor(mp), withShortTimer())

	for _, test := range processParentsTests {
		cp := Checkpoints{}
		if test.parentCheckpoint != "" {
			cp["shard-001"] = &Checkpoint{Checkpoint: test.parentCheckpoint}
		}
		if test.adjCheckpoint != "" {
			cp["shard-002"] = &Checkpoint{Checkpoint: test.adjCheckpoint}
		}

		sm := shardMap{
			"shard-001": mkClosedShard("shard-001", "100"),
			"shard-002": mkClosedShard("shard-002", "200"),
			"shard-003": mkOpenShard("shard-003"),
			"shard-004": mkOpenShard("shard-004"),
		}
		// shard-003 is a child of 001 and 002
		sm["shard-003"].Shard.ParentShardId = aws.String("shard-001")
		sm["shard-003"].Shard.AdjacentParentShardId = aws.String("shard-002")

		if test.parentRunning {
			// mark 001 as still being processed; should not start processing 003
			sm["shard-001"].processingRunning = true
		}

		// 004 isn't being processed and has no parents, so should have a processor started
		startCount := reader.startShardProcessors(cp, sm)
		if startCount != test.expectedStarted {
			t.Errorf("test=%q Incorrect start count expected=%d actual=%d",
				test.name, startCount, test.expectedStarted)
		}

		// we expected shard-001 and shard-004 to be marked active now
		var active []string
		for id, s := range sm {
			if s.processingRunning {
				active = append(active, id)
			}
		}

		sort.Strings(active)

		if !reflect.DeepEqual(active, test.expectedRunning) {
			t.Errorf("test=%q Incorrect shards being processed expected=%#v actual=%#v",
				test.name, test.expectedRunning, active)
		}
	}
}
