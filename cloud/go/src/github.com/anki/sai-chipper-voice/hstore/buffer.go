package hstore

import (
	"github.com/aws/aws-sdk-go/service/firehose"
)

type Buffer struct {
	streamName string
	buf        []*firehose.Record // array of bytes
	bufSize    int                // buf size in number of bytes
}

// Put data into the aggregator's buffer
func (a *Buffer) Put(record *firehose.Record) {
	a.buf = append(a.buf, record)
	a.bufSize += len(record.Data)
}

// Count returns number of records in the aggregator's buffer
func (a *Buffer) Count() int {
	return len(a.buf)
}

// Size returns number of bytes stored in the buffer
func (a *Buffer) Size() int {
	return a.bufSize
}

// clear aggregator's buffer
func (a *Buffer) clear() {
	a.bufSize = 0
	a.buf = make([]*firehose.Record, 0)
}

// Drain creates a Firehose batch and send the records into the Firehose stream
// Note, calling func needs to lock/unlock as needed
func (a *Buffer) Drain() []*firehose.Record {

	// TODO: return a checksum??

	records := make([]*firehose.Record, len(a.buf))
	copy(records, a.buf)
	a.clear()
	return records
}
