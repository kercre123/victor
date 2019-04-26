package hstore

import (
	"encoding/json"
	"errors"
	"strconv"
	"testing"
	"time"

	"github.com/anki/sai-service-framework/svc"
	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/service/firehose"
	"github.com/aws/aws-sdk-go/service/firehose/firehoseiface"
	"github.com/stretchr/testify/assert"
)

const (
	retryOKFailure   = 1
	retryFailFailure = 5
	logTranscript    = false
)

var (
	awsError = awserr.New(
		firehose.ErrCodeServiceUnavailableException,
		"The service is unavailable. Backoff and retry",
		errors.New("The service is unavailable. Backoff and retry"),
	)
	flushDuration svc.Duration
)

type mockFirehoseClient struct {
	firehoseiface.FirehoseAPI
	retry int
}

func batchOutput(b *firehose.PutRecordBatchInput, failures int) *firehose.PutRecordBatchOutput {
	failedCount := int64(failures)

	resp := make([]*firehose.PutRecordBatchResponseEntry, len(b.Records))
	for i := 0; i < len(b.Records); i++ {
		r := &firehose.PutRecordBatchResponseEntry{}
		if failures > 0 {
			// return record as failures, RecordId will be nil
			r.SetErrorCode(firehose.ErrCodeServiceUnavailableException)
			r.SetErrorMessage("The service is unavailable. Backoff and retry")
			failures--
		} else {
			r.SetRecordId(strconv.Itoa(1))
		}
		resp[i] = r
	}

	return &firehose.PutRecordBatchOutput{
		FailedPutCount:   &failedCount,
		RequestResponses: resp,
	}
}

func (m *mockFirehoseClient) PutRecordBatch(b *firehose.PutRecordBatchInput) (*firehose.PutRecordBatchOutput, error) {
	if *b.DeliveryStreamName == "test-success" {
		return batchOutput(b, 0), nil

	} else if *b.DeliveryStreamName == "test-retry-ok" {
		if m.retry > 0 {
			return batchOutput(b, 0), nil
		} else {
			// return some failures
			m.retry++
			return batchOutput(b, retryOKFailure), awsError
		}

	} else if *b.DeliveryStreamName == "test-retry-fail" {
		m.retry++
		return batchOutput(b, retryFailFailure), awsError
	} else if *b.DeliveryStreamName == "test-output-nil" {
		return &firehose.PutRecordBatchOutput{}, nil
	}

	return nil, errors.New("Invalid Test Stream")
}

//
// Client tests
//

func TestPutSuccess(t *testing.T) {
	mockFH := &mockFirehoseClient{}
	streamName := "test-success"
	flushDuration.Set("10s")
	cfg := &Config{
		StreamName:      &streamName,
		BufferFlushTime: &flushDuration,
	}

	c := NewClient(cfg, mockFH, logTranscript)
	c.Start()

	record := Record{Hypothesis: "set timer", RawEntity: "nothing"}
	err := c.Put(record)

	assert.Nil(t, err)
	time.Sleep(10 * time.Millisecond)

	assert.Equal(t, 1, c.bufferCount())

	c.Stop()
	time.Sleep(10 * time.Millisecond)

	assert.Equal(t, 0, c.bufferCount())
}

func TestPutClientStopped(t *testing.T) {
	mockFH := &mockFirehoseClient{}
	streamName := "test-success"
	flushDuration.Set("10s")
	cfg := &Config{
		StreamName:      &streamName,
		BufferFlushTime: &flushDuration,
	}

	c := NewClient(cfg, mockFH, logTranscript)

	record := Record{Hypothesis: "set timer", RawEntity: "nothing"}
	err := c.Put(record)
	assert.NotNil(t, err)
	assert.Equal(t, ErrClientStopped, err)
}

func TestPutRecordTooBig(t *testing.T) {
	mockFH := &mockFirehoseClient{}
	streamName := "test-success"
	flushDuration.Set("10s")
	cfg := &Config{
		StreamName:      &streamName,
		BufferFlushTime: &flushDuration,
	}

	c := NewClient(cfg, mockFH, logTranscript)
	c.Start()

	entity := "aaaaaaaaaabbbbbbbbbbccccccccdddddddddd"
	for i := 0; i < 15; i++ {
		entity += entity
	}
	record := Record{Hypothesis: "set timer", RawEntity: entity}
	err := c.Put(record)
	assert.NotNil(t, err)
	assert.Equal(t, ErrRecordSizeExceeded, err)
	c.Stop()
}

func TestLoopTimeoutSuccess(t *testing.T) {
	mockFH := &mockFirehoseClient{}
	streamName := "test-success"
	flushDuration.Set("100ms")
	cfg := &Config{
		StreamName:      &streamName,
		BufferFlushTime: &flushDuration,
	}

	c := NewClient(cfg, mockFH, logTranscript)
	c.Start()

	record := Record{Hypothesis: "set timer", RawEntity: "nothing"}
	err := c.Put(record)

	assert.Nil(t, err)
	time.Sleep(110 * time.Millisecond)
	assert.Equal(t, 0, c.bufferCount())
	c.Stop()
}

func TestLoopDrainDuringPut(t *testing.T) {
	mockFH := &mockFirehoseClient{}
	streamName := "test-success"
	flushDuration.Set("100s")
	cfg := &Config{
		StreamName:      &streamName,
		BufferFlushTime: &flushDuration,
	}

	c := NewClient(cfg, mockFH, logTranscript)
	c.Start()

	extras := 5
	for i := 0; i < maxFirehoseBatchRecords+extras; i++ {
		record := Record{Hypothesis: "set timer", RawEntity: strconv.Itoa(i)}
		err := c.Put(record)
		assert.Nil(t, err)
	}

	time.Sleep(10 * time.Millisecond)
	assert.Equal(t, extras, c.bufferCount())

	c.Stop()
}

//
// Firehose Tests
//

func TestFlushSuccess(t *testing.T) {
	mockFH := &mockFirehoseClient{}
	streamName := "test-success"
	cfg := &Config{StreamName: &streamName}

	c := NewClient(cfg, mockFH, logTranscript)

	nRec := 2
	records := make([]*firehose.Record, nRec)

	record := Record{Hypothesis: "set timer", RawEntity: strconv.Itoa(1)}
	data, _ := json.Marshal(record)
	records[0] = &firehose.Record{Data: data}

	record = Record{Hypothesis: "set timer", RawEntity: strconv.Itoa(2)}
	data, _ = json.Marshal(record)
	records[1] = &firehose.Record{Data: data}

	c.semaphore.acquire()
	nProc, _, nRetry := c.flush(records)
	assert.Equal(t, nRec, nProc)
	assert.Equal(t, 0, nRetry)
}

func TestFlushRetryOK(t *testing.T) {
	mockFH := &mockFirehoseClient{}
	streamName := "test-retry-ok"

	cfg := &Config{StreamName: &streamName}

	c := NewClient(cfg, mockFH, logTranscript)

	nRec := 3
	records := make([]*firehose.Record, nRec)
	for i := 0; i < nRec; i++ {
		record := Record{Hypothesis: "set timer", RawEntity: strconv.Itoa(i)}
		data, _ := json.Marshal(record)
		records[i] = &firehose.Record{Data: data}
	}

	c.semaphore.acquire()
	nProc, nSucc, nRetry := c.flush(records)
	// simulate one failure, therefore numProcessed = numRecords + 1
	assert.Equal(t, nRec+retryOKFailure, nProc)
	assert.Equal(t, 1, nRetry)
	assert.Equal(t, nRec, nSucc)

}

// TestFlushRetryFail tests for failures even with max number of retrys.
// Each batch has 5 failures
func TestFlushRetryFail(t *testing.T) {
	mockFH := &mockFirehoseClient{}
	streamName := "test-retry-fail"
	maxRetry := 3
	cfg := &Config{StreamName: &streamName, MaxRetry: &maxRetry}

	c := NewClient(cfg, mockFH, logTranscript)

	nRec := 10
	records := make([]*firehose.Record, nRec)
	for i := 0; i < nRec; i++ {
		record := Record{Hypothesis: "set timer", RawEntity: strconv.Itoa(i)}
		data, _ := json.Marshal(record)
		records[i] = &firehose.Record{Data: data}
	}

	c.semaphore.acquire()
	nProc, nSucc, nRetry := c.flush(records)

	expectedProc := retryFailFailure*(*c.MaxRetry) + nRec
	assert.Equal(t, expectedProc, nProc)
	assert.Equal(t, *c.MaxRetry+1, nRetry)
	assert.Equal(t, (nRec - retryFailFailure), nSucc)
}

func TestFlushLargeBatch(t *testing.T) {
	mockFH := &mockFirehoseClient{}
	streamName := "test-success"
	cfg := &Config{StreamName: &streamName}

	c := NewClient(cfg, mockFH, logTranscript)

	nRec := maxFirehoseBatchRecords + 5
	records := make([]*firehose.Record, nRec)
	for i := 0; i < nRec; i++ {
		record := Record{Hypothesis: "set timer", RawEntity: strconv.Itoa(i)}
		data, _ := json.Marshal(record)
		records[i] = &firehose.Record{Data: data}
	}

	c.semaphore.acquire()
	nProc, _, nRetry := c.flush(records)
	assert.Equal(t, nRec, nProc)
	assert.Equal(t, 0, nRetry)
}

func TestPutResponseNil(t *testing.T) {
	mockFH := &mockFirehoseClient{}
	streamName := "test-output-nil"
	maxRetry := 2
	cfg := &Config{
		StreamName: &streamName,
		MaxRetry:   &maxRetry,
	}

	c := NewClient(cfg, mockFH, logTranscript)

	nRec := 5
	records := make([]*firehose.Record, nRec)
	for i := 0; i < nRec; i++ {
		record := Record{Hypothesis: "set timer", RawEntity: strconv.Itoa(i)}
		data, _ := json.Marshal(record)
		records[i] = &firehose.Record{Data: data}
	}

	c.semaphore.acquire()
	nProc, nSuccess, nRetry := c.flush(records)
	assert.Equal(t, nRec*(maxRetry+1), nProc)
	assert.Equal(t, maxRetry+1, nRetry)
	assert.Equal(t, 0, nSuccess)
}
