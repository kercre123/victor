package hstore

import (
	"encoding/json"
	"testing"

	"github.com/aws/aws-sdk-go/service/firehose"
	"github.com/stretchr/testify/assert"
	"strconv"
)

var (
	HRecord    Record = Record{Hypothesis: "set timer"}
	StreamName string = "test-stream"
)

func TestBufferPut(t *testing.T) {

	a := Buffer{streamName: StreamName}
	record := Record{Hypothesis: "set timer"}
	data, _ := json.Marshal(record)
	a.Put(&firehose.Record{Data: data})

	assert.Equal(t, 1, a.Count())
	assert.Equal(t, len(data), a.Size())

	b := a.Drain()
	assert.Equal(t, 0, a.Count())
	assert.Equal(t, 0, a.Size())
	assert.Equal(t, 1, len(b))
}

func TestBufferLotsOfPuts(t *testing.T) {
	a := Buffer{streamName: StreamName}
	extras := 2
	nBatch := 3
	nRecords := (nBatch * maxFirehoseBatchRecords) + extras
	nBytes := 0
	for i := 0; i < nRecords; i++ {
		record := Record{Hypothesis: "set timer", RawEntity: strconv.Itoa(i)}
		data, _ := json.Marshal(record)
		a.Put(&firehose.Record{Data: data})
		nBytes += len(data)
	}

	assert.Equal(t, nRecords, a.Count())
	assert.Equal(t, nBytes, a.Size())

	b := a.Drain()
	assert.Equal(t, 0, a.Count())
	assert.Equal(t, 0, a.Size())
	assert.Equal(t, nRecords, len(b))
}
