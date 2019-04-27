// Copyright 2016 Anki, Inc.

package logsender

import (
	"errors"
	"fmt"
	"io"
	"os"
	"reflect"
	"sort"
	"syscall"
	"testing"
	"time"

	"github.com/anki/sai-kinlog/logsender/logrecord"
	"github.com/aws/aws-sdk-go/service/kinesis"
)

func crash() {
	syscall.Kill(os.Getpid(), syscall.SIGABRT)
}

func validateRecordData(recordData []byte, expectedSource string, expectedData ...string) error {
	dec, err := logrecord.NewDecoder(recordData)
	if err != nil {
		return fmt.Errorf("Unexpected record decoder error: %s", err)
	}
	if source := dec.Metadata().Source; source != expectedSource {
		return fmt.Errorf("Incorrect record metadata source. expected=%s actual=%s", expectedSource, source)
	}
	for i, expected := range expectedData {
		var data logrecord.Data
		if err := dec.Decode(&data); err != nil {
			return fmt.Errorf("Failed to decode data offset=%d error=%s", i, err)
		}
		if actual := string(data.Data); actual != expected {
			return fmt.Errorf("Incorrect data for offset=%d expected=%q actual=%q", i, expected, actual)
		}
	}
	return nil
}

// extract the events portion of each supplied record
func extractRecordData(recordData []byte) (events []string, err error) {
	dec, err := logrecord.NewDecoder(recordData)
	if err != nil {
		return nil, fmt.Errorf("Unexpected record decoder error: %s", err)
	}
	for {
		var data logrecord.Data
		if err := dec.Decode(&data); err == io.EOF {
			return events, nil
		} else if err != nil {
			return nil, fmt.Errorf("Failed to decode data error=%s", err)
		}
		events = append(events, string(data.Data))
	}
}

func extractRecordsData(records ...*kinesis.PutRecordsRequestEntry) (events []string, err error) {
	for _, rec := range records {
		subevents, err := extractRecordData(rec.Data)
		if err != nil {
			return nil, err
		}
		events = append(events, subevents...)
	}
	return events, nil
}

// trigger a timed flush by sending tick notifications to
// the supplied timeCh every couple of milliseconds
// until a flush notification is sent to the flushStarted chan
//
// the first tick may not trigger a flush if a flush is already
// in progress, of the flusher goroutine has not yet started.
func startFlush(tickCh chan time.Time, flushStarted chan bool) error {
	start := time.Now()
	i := 0
	for time.Since(start) < 5*time.Second {
		i++
		tickCh <- time.Now()
		select {
		case <-flushStarted:
			fmt.Println("START", i)
			return nil
		case <-time.After(2 * time.Millisecond):
		}
	}
	return errors.New("timeout waiting for flush to start")
}

func TestSenderFlushTimer(t *testing.T) {
	tickCh := make(chan time.Time)
	flushed := make(chan *kinesis.PutRecordsInput, 1)
	k := new(fakeKinesis)

	flushStarted := make(chan bool)
	k.customResponder = func(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error) {
		flushStarted <- true
		flushed <- input
		return &kinesis.PutRecordsOutput{Records: []*kinesis.PutRecordsResultEntry{kinPutOK}}, nil
	}

	s, err := New("test_stream",
		WithKinesis(k),
		withFlushTicker(tickCh),
		WithNoConnectivityCheck())
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	defer s.Stop()

	s.write(logrecord.LogEntry{
		Metadata: logrecord.Metadata{Source: "source0"},
		Data:     logrecord.Data{Data: []byte("data0")},
	})

	if err := startFlush(tickCh, flushStarted); err != nil {
		t.Fatal(err) // timeout
	}

	put := <-flushed
	if len(put.Records) != 1 {
		t.Fatalf("Incorrect record count flushed expected=1 actual=%d", len(put.Records))
	}
	if err := validateRecordData(put.Records[0].Data, "source0", "data0"); err != nil {
		t.Fatal("Incorrect data flushed: ", err)
	}
}

func TestSenderFlushOnFull(t *testing.T) {
	tickCh := make(chan time.Time)

	flushed := make(chan *kinesis.PutRecordsInput, 2)
	k := new(fakeKinesis)

	k.customResponder = func(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error) {
		flushed <- input
		return &kinesis.PutRecordsOutput{Records: []*kinesis.PutRecordsResultEntry{kinPutOK}}, nil
	}
	s, err := New("test_stream",
		WithKinesis(k),
		withFlushTicker(tickCh),
		withMaxObjectsPerPut(1),
		WithNoConnectivityCheck())

	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	// wait long enough for flush goroutine to start
	// TODO: need a deterministic way to do this
	time.Sleep(10 * time.Millisecond)
	defer s.Stop()

	s.write(logrecord.LogEntry{
		Metadata: logrecord.Metadata{Source: "source0"},
		Data:     logrecord.Data{Data: []byte("data0")},
	})

	// second write should trigger the flush
	s.write(logrecord.LogEntry{
		Metadata: logrecord.Metadata{Source: "source1"},
		Data:     logrecord.Data{Data: []byte("data1")},
	})

	// We never fire the timer, so that can't have triggered the flush
	select {
	case put := <-flushed:
		if len(put.Records) != 1 { // different metadata == separate records
			t.Fatalf("Incorrect record count flushed expected=2 actual=%d", len(put.Records))
		}
		if err := validateRecordData(put.Records[0].Data, "source0", "data0"); err != nil {
			t.Fatal("Incorrect data flushed: ", err)
		}

	case <-time.After(5 * time.Second):
		t.Fatal("Time out waiting for flush")
	}
}

// if all flushers are busy, the sender should continue buffering
func TestSenderBufferOnBusy(t *testing.T) {
	tickCh := make(chan time.Time, 1)

	k := new(fakeKinesis)
	flushed := make(chan *kinesis.PutRecordsInput, 10)
	flushStarted := make(chan bool, 10)
	release := make(chan struct{})

	putCalls := 0
	putRecords := 0
	k.customResponder = func(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error) {
		// wait until there's an item in the buffer, but don't read
		flushStarted <- true
		<-release

		flushed <- input
		putCalls++
		putRecords += len(input.Records)
		return &kinesis.PutRecordsOutput{}, nil
	}

	s, err := New("test_stream",
		WithKinesis(k),
		MaxParallelFlush(1),
		withFlushTicker(tickCh),
		withMaxObjectsPerPut(3),
		withMaxSoftRecordSize(20),
		WithNoConnectivityCheck())

	if err != nil {
		close(release)
		t.Fatal("Unexpected error", err)
	}
	defer s.Stop()

	// push an event in and trigger a flush
	s.write(logrecord.LogEntry{
		Metadata: logrecord.Metadata{Source: "source0"},
		Data:     logrecord.Data{Data: []byte("data0")},
	})

	if err := startFlush(tickCh, flushStarted); err != nil {
		close(release)
		t.Fatal(err)
	}

	// trigger a bunch more flushes; shouldn't start any and should continue buffering
	// until maxObjectsPerPut is reached
	var i int
	for i = 0; i < 5; i++ {
		err := s.write(logrecord.LogEntry{
			Metadata: logrecord.Metadata{Source: fmt.Sprintf("source%d", i+1)},
			Data:     logrecord.Data{Data: []byte(fmt.Sprintf("write%d", i))},
		})
		if err == errPendingIsFull {
			break
		}
		if err != nil {
			close(release)
			t.Fatalf("Unexpected write error on i=%d error=%v", i, err)
		}
	}
	if i != 3 {
		close(release)
		t.Fatalf("Wrote incorrect number of entries before filling pending.  expected=%d actual=%d", 3, i)
	}
	tickCh <- time.Now()
	close(release)
	s.Stop()

	if putCalls != 2 {
		// should of sent the one outstanding, plus the filled buffer once
		// the release channel was closed
		t.Errorf("Incorrect number of put calls expected=%d actual=%d", 3, putCalls)
	}

	if putRecords != 4 {
		// should of sent the first entry from the first flush, and then the
		// three buffered records in the second put
		t.Errorf("Incorrect number of put records expected=%d actual=%d", 3, putRecords)
	}

}

// if the flusher gets a hard error, it should ask the error handler for direction
func TestSenderHandleHardError(t *testing.T) {
	tickCh := make(chan time.Time)

	discard := errors.New("discard")
	retry := errors.New("retry")
	putCalls := 0
	k := new(fakeKinesis)
	k.customResponder = func(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error) {
		//flushed <- input
		putCalls++
		if putCalls == 1 {
			return nil, retry
		}
		return nil, discard
	}

	ehCalls := 0
	eh := ErrorHandlerFunc(func(s *Sender, err error) FlushFailureAction {
		ehCalls++
		switch err {
		case retry:
			return FlushRetry
		case discard:
			return FlushDiscard
		default:
			panic(fmt.Sprintf("Unknown error: %#v", err))
		}
	})

	s, err := New("test_stream",
		WithKinesis(k),
		WithErrorHandler(eh),
		withFlushTicker(tickCh),
		WithNoConnectivityCheck())
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	defer s.Stop()

	s.write(logrecord.LogEntry{
		Metadata: logrecord.Metadata{Source: "source0"},
		Data:     logrecord.Data{Data: []byte("data0")},
	})

	// trigger a flush
	fmt.Printf("XX3 %p\n", s)
	tfired := make(chan struct{}, 1)
	go func() {
		tickCh <- time.Now()
		tfired <- struct{}{}
	}()

	// wait for shutdown, indicating that all flushers have exited
	s.Stop()

	// expect the error handler & putrecords to be called twice only
	if ehCalls != 2 {
		t.Errorf("ehCalls=%d expected=%d", ehCalls, 2)
	}

	if putCalls != 2 {
		t.Errorf("putCalls=%d expected=%d", putCalls, 2)
	}
}

// Check that all outstanding data is flushed before Stop() returns.
func TestSenderStopFlush(t *testing.T) {
	tickCh := make(chan time.Time, 1)

	flushed := make(chan *kinesis.PutRecordsInput, 10)
	releasePut := make(chan struct{})
	k := new(fakeKinesis)

	k.customResponder = func(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error) {
		flushed <- input
		<-releasePut
		return &kinesis.PutRecordsOutput{Records: []*kinesis.PutRecordsResultEntry{kinPutOK}}, nil
	}

	s, err := New("test_stream",
		WithKinesis(k),
		withFlushTicker(tickCh),
		WithNoConnectivityCheck())
	if err != nil {
		close(releasePut)
		t.Fatal("Unexpected error", err)
	}

	// load data causing all flush goroutines to be active, and blocked
	// and add more data into the buffer behind that
	var i int
	var seenData []string
	var expected []string
	for i = 0; i < 10; i++ {
		expected = append(expected, fmt.Sprintf("data%d", i))
		err := s.write(logrecord.LogEntry{
			Metadata: logrecord.Metadata{Source: fmt.Sprintf("source%d", i)},
			Data:     logrecord.Data{Data: []byte(fmt.Sprintf("data%d", i))},
		})
		if err == ErrQueueFull {
			break
		} else if err != nil {
			close(releasePut)
			t.Fatal("Unexpected error", err)
		}
		if i < s.maxParallelFlush {
			select {
			// trigger flush
			case tickCh <- time.Now():
				time.Sleep(2 * time.Millisecond) // wait for flush to start

			// wait for put
			case put := <-flushed:
				events, err := extractRecordsData(put.Records...)
				if err != nil {
					close(releasePut)
					t.Fatal(err)
				}
				seenData = append(seenData, events...)
			case <-time.After(5 * time.Second):
				close(releasePut)
				t.Fatal("Timeout waiting for PutRecords to be called", i)
			}
		}
	}

	// call Stop() and check it doesn't return until all data has been flushed
	done := make(chan struct{}, 1)
	go func() {
		s.Stop()
		done <- struct{}{}
	}()

	close(releasePut) // let out customResponder flushes complete
	select {
	case <-done: // Stop() returned
	case <-time.After(5 * time.Second):
		t.Fatal("Timeout waiting for Stop")
	}

	// collect the remaining flushed data
	close(flushed)
	for put := range flushed {
		events, err := extractRecordsData(put.Records...)
		if err != nil {
			t.Fatal(err)
		}
		seenData = append(seenData, events...)
	}

	// seenData should now have all 10 events
	sort.Strings(seenData)
	if !reflect.DeepEqual(seenData, expected) {
		t.Errorf("Incorrect/incomplete data flushed after Stop() expected=%#v actual=%#v",
			expected, seenData)
	}
}

func TestSenderPostStop(t *testing.T) {
	k := new(fakeKinesis)
	s, err := New("test_stream",
		WithKinesis(k),
		WithNoConnectivityCheck())
	if err != nil {
		t.Fatal("Unexpected error", err)
	}

	stopped := make(chan struct{}, 1)
	go func() {
		s.Stop()
		stopped <- struct{}{}
	}()

	select {
	case <-stopped:
	case <-time.After(5 * time.Second):
		t.Fatal("Stop failed")
	}

	// writes should panic now it's stopped
	var didPanic bool
	func() {
		defer func() {
			if x := recover(); x != nil {
				didPanic = true
			}
		}()
		s.write(logrecord.LogEntry{
			Metadata: logrecord.Metadata{Source: "source0"},
			Data:     logrecord.Data{Data: []byte("data0")},
		})
	}()

	if !didPanic {
		t.Fatal("Write after stop did not trigger a panic")
	}
}
