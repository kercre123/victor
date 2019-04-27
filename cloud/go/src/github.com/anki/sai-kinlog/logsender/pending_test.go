// Copyright 2016 Anki, Inc.

package logsender

import (
	"bytes"
	"compress/gzip"
	"encoding/json"
	"errors"
	"fmt"
	"reflect"
	"sort"
	"testing"

	"github.com/anki/sai-kinlog/logsender/logrecord"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/kinesis"
	"github.com/cenkalti/backoff"
)

func TestPendingAddEntry(t *testing.T) {
	pool := newEncoderPool()
	pi := newPendingItems(pool, defaultPendingSizeLimits)
	md1 := logrecord.Metadata{Source: "source1"}
	md2 := logrecord.Metadata{Source: "source2"}
	events := []logrecord.LogEntry{
		{Metadata: md1, Data: logrecord.Data{Data: []byte("md1/event1")}},
		{Metadata: md2, Data: logrecord.Data{Data: []byte("md2/event1")}},
		{Metadata: md1, Data: logrecord.Data{Data: []byte("md1/event2")}},
	}
	for _, ev := range events {
		if err := pi.addEntry(ev, true); err != nil {
			t.Fatalf("Failed to add event %#v: %s", ev, err)
		}
	}

	if len(pi.records) != 2 {
		t.Fatalf("Incorrect buffer count expected=%d actual=%d", 2, len(pi.records))
	}

	var seen1, seen2 bool
	for md, b := range pi.records {
		if md.Source == "source1" {
			if err := testBufferMatchesEvents(b, []logrecord.LogEntry{events[0], events[2]}, true); err != nil {
				t.Errorf("Source1 buffer incorrect: %s", err)
			}
			seen1 = true
		} else {
			if err := testBufferMatchesEvents(b, []logrecord.LogEntry{events[1]}, true); err != nil {
				t.Errorf("Source2 buffer incorrect: %s", err)
			}
			seen2 = true
		}
	}
	if !seen1 {
		t.Fatal("did not see source1")
	}
	if !seen2 {
		t.Fatal("did not see source2")
	}
}

func TestPendingAddFullSize(t *testing.T) {
	sizes := defaultPendingSizeLimits
	sizes.maxPendingSize = 17
	pi := newPendingItems(newEncoderPool(), sizes)

	for i := 0; i < 2; i++ {
		err := pi.addEntry(logrecord.LogEntry{
			Metadata: logrecord.Metadata{Source: fmt.Sprintf("source%d", i)},
			Data:     logrecord.Data{Data: []byte(fmt.Sprintf("event%d", i))},
		}, true)
		if err != nil {
			t.Fatal("Failed to add entry", err)
		}
	}

	// next should fail
	err := pi.addEntry(logrecord.LogEntry{
		Metadata: logrecord.Metadata{Source: "fail"},
		Data:     logrecord.Data{Data: []byte("event3")},
	}, true)
	if err != errPendingIsFull {
		t.Error("Did not get pendingIsFull error, got ", err)
	}
}

// make sure the soft and hard limit sizes are respected for an
// individual encoder.
func TestPendingAddEncoderSize(t *testing.T) {
	pi := newPendingItems(newEncoderPool(), defaultPendingSizeLimits)
	rec := logrecord.LogEntry{
		Metadata: logrecord.Metadata{Source: "small"},
		Data:     logrecord.Data{Data: bytes.Repeat([]byte("x"), defaultPendingSizeLimits.maxSoftRecordSize+1)},
	}
	if err := pi.addEntry(rec, true); err != errReachedSoftLimit {
		t.Fatalf("Did not get expected error, got error=%#v", err)
	}

	// sending with soft limits disabled should work
	if err := pi.addEntry(rec, false); err != nil {
		t.Fatal("Unexpected error", err)
	}

	// hitting the hard limit should return an error
	rec.Data.Data = bytes.Repeat([]byte("x"), logrecord.MaxRecordSize-defaultPendingSizeLimits.maxSoftRecordSize)
	if err := pi.addEntry(rec, false); err != logrecord.ErrRecordTooLarge {
		t.Fatalf("Did not get expected error, got error=%#v", err)
	}
}

func TestPendingAddFullCount(t *testing.T) {
	sizes := defaultPendingSizeLimits
	sizes.maxObjectsPerPut = 2
	pi := newPendingItems(newEncoderPool(), sizes)

	for i := 0; i < 2; i++ {
		err := pi.addEntry(logrecord.LogEntry{
			Metadata: logrecord.Metadata{Source: fmt.Sprintf("source%d", i)},
			Data:     logrecord.Data{Data: []byte(fmt.Sprintf("event%d", i))},
		}, true)
		if err != nil {
			t.Fatal("Failed to add entry", err)
		}
	}

	// next should fail
	err := pi.addEntry(logrecord.LogEntry{
		Metadata: logrecord.Metadata{Source: "fail"},
		Data:     logrecord.Data{Data: []byte("event3")},
	}, true)
	if err != errPendingIsFull {
		t.Error("Did not get pendingIsFull error, got ", err)
	}
}

var kinPutOK = new(kinesis.PutRecordsResultEntry)
var kinPutFailed = &kinesis.PutRecordsResultEntry{ErrorCode: aws.String("Failed")}

func recsToSources(recs []*kinesis.PutRecordsRequestEntry) (sources []string, err error) {
	for i, rec := range recs {
		var md logrecord.Metadata
		r, _ := gzip.NewReader(bytes.NewBuffer(rec.Data))
		dec := json.NewDecoder(r)
		if err := dec.Decode(&md); err != nil {
			return nil, fmt.Errorf("rec=%d decoder failure=%v", i, err)
		}
		sources = append(sources, md.Source)
	}
	sort.Strings(sources)
	return sources, nil
}

func TestPendingFlushOK(t *testing.T) {
	pi := newPendingItems(newEncoderPool(), defaultPendingSizeLimits)

	// add some records to flush
	var results []*kinesis.PutRecordsResultEntry
	for i := 0; i < 3; i++ {
		err := pi.addEntry(logrecord.LogEntry{
			Metadata: logrecord.Metadata{Source: fmt.Sprintf("source%d", i)},
			Data:     logrecord.Data{Data: []byte(fmt.Sprintf("event%d", i))},
		}, true)
		if err != nil {
			t.Fatal("Failed to add entry", err)
		}
		results = append(results, kinPutOK) // no error
	}

	// flush
	k := &fakeKinesis{
		responses: []kresp{
			{ok: &kinesis.PutRecordsOutput{
				Records: results,
			}},
		},
	}
	if _, err := pi.flushToKinesis(k, "test_stream"); err != nil {
		t.Fatal("Unexpected error on flush", err)
	}
	if len(k.calls) != 1 {
		t.Fatal("Incorrect number of calls to PutRecords", len(k.calls))
	}

	// check call
	if rc := len(k.calls[0].Records); rc != 3 {
		t.Fatal("Incorrect number of records", rc)
	}

	sources, err := recsToSources(k.calls[0].Records)
	if err != nil {
		t.Fatal(err)
	}

	expected := []string{"source0", "source1", "source2"}
	if !reflect.DeepEqual(sources, expected) {
		t.Errorf("Incorrect sources sent.  expected=%#v actual=%#v", expected, sources)
	}
}

// If a couple of the records fails to send, it should retry those two only
func TestPendingFlushRetry(t *testing.T) {
	pi := newPendingItems(newEncoderPool(), defaultPendingSizeLimits)
	pi.backoff = new(backoff.ZeroBackOff) // don't wait for retry

	// create some recordEncoders to put
	failedMap := make(map[string]bool)
	for i := 0; i < 4; i++ {
		s := fmt.Sprintf("source%d", i)
		b := new(logrecord.Encoder)
		md := logrecord.Metadata{Source: s}
		b.Reset(md)
		pi.records[md] = b
		// have the customResponder mark source0 and source2 as put failures
		if i%2 == 0 {
			failedMap[b.PartitionKey()] = true
		}
	}

	// flush
	k := new(fakeKinesis)

	k.customResponder = func(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error) {
		if len(k.calls) == 1 {
			result := &kinesis.PutRecordsOutput{FailedRecordCount: aws.Int64(2)}
			for _, rec := range input.Records {
				if failedMap[aws.StringValue(rec.PartitionKey)] {
					result.Records = append(result.Records, kinPutFailed)
				} else {
					result.Records = append(result.Records, kinPutOK)
				}
			}
			return result, nil
		} else if len(k.calls) == 2 {
			// success on the second attempt
			return &kinesis.PutRecordsOutput{
				Records: []*kinesis.PutRecordsResultEntry{kinPutOK, kinPutOK},
			}, nil
		} else {
			panic("More than two calls to PutRecords")
		}
	}

	if _, err := pi.flushToKinesis(k, "test_stream"); err != nil {
		t.Fatal("Unexpected error on flush", err)
	}

	if len(k.calls) != 2 {
		t.Fatal("Incorrect number of calls to PutRecords", len(k.calls))
	}

	// check first call
	if rc := len(k.calls[0].Records); rc != 4 {
		t.Fatal("Incorrect number of records", rc)
	}

	sources, err := recsToSources(k.calls[0].Records)
	if err != nil {
		t.Fatal(err)
	}

	expected := []string{"source0", "source1", "source2", "source3"}
	if !reflect.DeepEqual(sources, expected) {
		t.Errorf("Incorrect sources sent.  expected=%#v actual=%#v", expected, sources)
	}

	// second call should only come from source1 and source3 as we forced those to fail
	if rc := len(k.calls[1].Records); rc != 2 {
		t.Fatal("Incorrect number of records", rc)
	}

	sources, err = recsToSources(k.calls[1].Records)
	if err != nil {
		t.Fatal(err)
	}

	expected = []string{"source0", "source2"}
	sort.Strings(expected)
	if !reflect.DeepEqual(sources, expected) {
		t.Errorf("Incorrect sources sent for second call.  expected=%#v actual=%#v", expected, sources)
	}
}

// An error from PutRecords should immediately bubble up
func TestPendingFlushHardError(t *testing.T) {
	pi := newPendingItems(newEncoderPool(), defaultPendingSizeLimits)
	pi.backoff = new(backoff.ZeroBackOff) // don't wait for retry
	b := new(logrecord.Encoder)
	md := logrecord.Metadata{Source: "test"}
	b.Reset(md)
	pi.records[md] = b

	k := new(fakeKinesis)
	callCount := 0
	putError := errors.New("hard error")
	k.customResponder = func(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error) {
		callCount++
		return nil, putError
	}

	softErrs, hardErr := pi.flushToKinesis(k, "test_stream")
	if hardErr == nil {
		t.Fatal("No error returned from flushToKinesis")
	}

	if hardErr != putError {
		t.Errorf("Incorrect error returned: %v", hardErr)
	}

	if len(softErrs) != 0 {
		t.Errorf("Unexpected soft errors: %#v", softErrs)
	}

	if callCount != 1 {
		t.Errorf("Expected 1 call to PutRecords, actual=%d", callCount)
	}
}

func TestPendingBufferReuse(t *testing.T) {
	// While this test works with racemode disabled (sync.Pool is disabled when the
	// race detector is enabled), it can't be promised to be reliable even without the
	// race detector as the runtime is free to discard the contents of the pool at any time
	//
	// Maybe there's a beter way to test this..
	t.Skip()

	pi := newPendingItems(newEncoderPool(), defaultPendingSizeLimits)

	var results []*kinesis.PutRecordsResultEntry
	for i := 0; i < 4; i++ {
		err := pi.addEntry(logrecord.LogEntry{
			Metadata: logrecord.Metadata{Source: fmt.Sprintf("source%d", i)},
			Data:     logrecord.Data{Data: []byte(fmt.Sprintf("event%d", i))},
		}, true)
		if err != nil {
			t.Fatal("Failed to add entry", err)
		}
		results = append(results, &kinesis.PutRecordsResultEntry{}) // no error
	}
	bufs := make(map[*logrecord.Encoder]bool)
	for _, buf := range pi.records {
		//bufs = append(bufs, buf)
		bufs[buf] = true
	}

	k := &fakeKinesis{
		responses: []kresp{
			{ok: &kinesis.PutRecordsOutput{
				Records: results,
			}},
		},
	}

	if _, err := pi.flushToKinesis(k, "test_stream"); err != nil {
		t.Fatal("Unexpected error on flush", err)
	}

	// Should be able to get 4 bufs from the pool now that match those that were in use
	for i := 0; i < 4; i++ {
		buf := pi.recPool.Get().(*logrecord.Encoder)
		if !bufs[buf] {
			t.Errorf("i=%d buffer %p not in pool", i, buf)
		} else {
			delete(bufs, buf)
		}
	}
}

func testBufferMatchesEvents(b *logrecord.Encoder, events []logrecord.LogEntry, checkDone bool) error {
	md := events[0].Metadata
	r, err := gzip.NewReader(bytes.NewBuffer(b.Data()))
	if err != nil {
		return fmt.Errorf("gzip init failed: %s", err)
	}
	dec := json.NewDecoder(r)

	md.EncoderID = logrecord.EncoderID
	var outmd logrecord.Metadata
	if err := dec.Decode(&outmd); err != nil {
		return fmt.Errorf("metadata read failed: %s", err)
	}
	if !reflect.DeepEqual(md, outmd) {
		return fmt.Errorf("metadata mismatch expected=%#v actual=%#v", md, outmd)
	}

	for i := 0; i < len(events); i++ {
		var outev logrecord.Data
		if err := dec.Decode(&outev); err != nil {
			return fmt.Errorf("event read failed: %s", err)
		}
		if !reflect.DeepEqual(outev, events[i].Data) {
			return fmt.Errorf("idx=%d event mismatch:\nexpected=%#v (data: %q)\nactual=  %#v (data: %q)",
				i, events[i].Data, string(events[i].Data.Data), outev, string(outev.Data))
		}
	}

	if checkDone {
		var v interface{}
		if err := dec.Decode(&v); err == nil {
			return fmt.Errorf("Unexpected trailing data: %#v", v)
		}
	}
	return nil
}
