// Copyright 2016 Anki, Inc.

/*
Package logrecord encodes and decodes streams of log data into Kinesis
records.

A single Kinesis record can hold zero of more segments of log data, all of
which are in-order and share the same metadata (ie. came from the same
source, sourcetype and host and is destined for the same index).

The encoder handles both "broken" and "unbroken" log data streams, but
uses a different algorithm to select a partition key for each.

"broken" means that the log data is pre split-up into individual log entries
(even if they're multiline log entries) such that the receiver does not need
to process the events in order for them to be processed correctly.

"unbroken" events require strict ordering on the receiving end so that the
downstream log processor can correctly locate the beginning and end of
individual events according to its own parser (eg. by locating a datestamp
in the event).

Each record is allocated a Partition ID which Kinesis hashes to select a
shard to distribute the record to.  Broken event streams receive a random
partition id for each record, which allows for records to evenly
distributed amongst the shards within a Kinesis stream, maximizing
throughput.

Unbroken streams use a partition id that's unique to the metadata supplied
for it, ensuring that all records for the same metadata receive the same
partition id and so all records go the same shard.  This ensures that
the same downstream shard processor will process each record in the shard
and can feed an ordered set of records to the log processor for event
breaking.

Additionally to support reconstruction of ordered unbroken log streams
each record contains an EncoderID, which is randomly selected at program
start, and a BlockID, which is incremented for every subsequent record
for an unbroken data stream.


Record Format

Each record consists of a gzipped stream of JSON objects.  The first object
is always a Metadata object, followed by zero or more Data objects.
*/
package logrecord

import (
	"bytes"
	"compress/gzip"
	"crypto/md5"
	"crypto/rand"
	"encoding/json"
	"errors"
	"fmt"
	"math"
	"math/big"
	mathrand "math/rand"
	"strconv"
	"sync"
	"time"
)

var (
	// ErrRecordTooLarge is returned if adding data to a RecordEncoder would
	// cause it to exceed its maximum capacity.
	ErrRecordTooLarge = errors.New("record too large")
)

var (
	// EncoderID is randomly generated at startup and used for sequencing
	// unbroken stream pieces.
	EncoderID int64

	// MaxRecordSize specifies the maximum amount of uncompressed data to
	// store in a Kinesis record.  Due to compression the final size will
	// be substantially smaller than this.
	//
	// Note: Kinesis allows a maximum of 1MB of raw data (after the
	// encoder's gzip compression is complete) per record.  Setting
	// this value too high may cause records to exceed Kinesis limits.
	MaxRecordSize = 1024 * 1024
)

var (
	sequence = newSequenceGenerator()
)

func init() {
	rnd, err := rand.Int(rand.Reader, big.NewInt(math.MaxInt64))
	if err != nil {
		panic(err)
	}
	EncoderID = rnd.Int64()
}

type sequenceGenerator struct {
	m        sync.Mutex
	counters map[string]int64
}

func newSequenceGenerator() *sequenceGenerator {
	return &sequenceGenerator{
		counters: make(map[string]int64),
	}
}

func (s *sequenceGenerator) next(key string) int64 {
	s.m.Lock()
	defer s.m.Unlock()
	s.counters[key]++
	return s.counters[key]
}

type jsonEncoder interface {
	Encode(v interface{}) error
}

// Metadata defines metadata about a single log event stream.
type Metadata struct {
	// IsUnbroken determines whether individual data entries hold a complete
	// (pre-broken) event, or whether the stream is unbroken and requires
	// breaking multiline entries into individual events by Splunk
	IsUnbroken bool `json:"unbroken"`

	// Source defines where the data came from; eg. a pathname on disk.
	Source string `json:"source"`

	// Sourcetype defines a name for the format of the log data such as access_combined
	Sourcetype string `json:"sourcetype"`

	// Host holds the name of the machine from which the data originated.
	Host string `json:"host"`

	// Index optionally specifies the name of the Splunk index to send data to.
	Index string `json:"index"`

	// EncoderID is a random integer that an individual encoder process picks at startup.
	EncoderID int64 `json:"enc_id"`

	// BlockID is specified for unbroken event streams.  It's incremented by
	// one for each block of data for a specified source,sourcetype,host stream
	// of data to ensure it can be replayed in order on the receiving system.
	BlockID int64 `json:"block_id"`
}

// Data holds data about a single log event for broken streams, or a block of
// event text for unbroken streams.
type Data struct {
	// IsDone is set to true when the current end of an event stream is reached.
	// Splunk uses this information to determine that it's definitely reached an event
	// boundary and may switch processing to a different indexer.
	IsDone bool `json:"is_done"`

	// Timestamp specifies the time of the event.  If empty then Splunk will attempt
	// to extract the timestamp from the text of the event.
	Time *time.Time `json:"ts,omitempty"`

	// Data holds the event text.  It holds a single (possibly multiline) event if
	// the streams IsUnbroken metadata is set to false, else an arbitrary chunk of text.
	Data []byte `json:"data"`
}

// LogEntry holds the complete metadata and data for a log entry.
type LogEntry struct {
	Metadata Metadata
	Data     Data
}

// ApproxSize returns the  approximate size of the encoded entry
func (e *LogEntry) ApproxSize() int {
	// TODO: come up with a better guess!
	return len(e.Data.Data) + 20
}

// An Encoder encodes a single Kinesis record containing
// a number of compressed events that share common metadata.
//
// An encoded record consists of a gzipped stream of JSON objects.
// The JSON object stream starts with a Metadata object followed by
// zero or more EntryData objects.
//
// An uninitialized instance of an Encoder can be prepared for first use
// by calling Reset.  Alternatively, use NewEncoder.
type Encoder struct {
	partitionKey string
	rawSize      int
	buf          bytes.Buffer
	jw           *json.Encoder
	gzw          *gzip.Writer
}

// NewEncoder creates and initializes (resets) an Encoder ready for use.
func NewEncoder(md Metadata) *Encoder {
	e := new(Encoder)
	e.Reset(md)
	return e
}

// RawSize returns the number of bytes sent to Encode so far.
func (e *Encoder) RawSize() int {
	return e.rawSize
}

// Data returns the compressed binary data for the record.
func (e *Encoder) Data() []byte {
	e.gzw.Close()
	return e.buf.Bytes()
}

// PartitionKey returns the generated partition key representing
// the metadata encoded for the record.
func (e *Encoder) PartitionKey() string {
	return e.partitionKey
}

// Encode adds data for a single event entry.
func (e *Encoder) Encode(data Data) error {
	if e.partitionKey == "" {
		panic("AddData called before Reset")
	}
	size := len(data.Data)
	if e.rawSize+size > MaxRecordSize {
		return ErrRecordTooLarge
	}
	e.rawSize += size
	return e.jw.Encode(data)
}

// Reset resets the internal state of the encoder so it can be reused.
func (e *Encoder) Reset(md Metadata) {
	e.partitionKey = e.calcPartitionKey(md)
	e.buf.Reset()
	e.rawSize = 0
	if e.gzw == nil {
		e.gzw = gzip.NewWriter(&e.buf)
		e.jw = json.NewEncoder(e.gzw)
	} else {
		// save allocating a new compressor and json encoder
		e.gzw.Reset(&e.buf)
	}
	// write metadata as a header
	md.EncoderID = EncoderID
	if md.IsUnbroken {
		md.BlockID = sequence.next(fmt.Sprintf("%s::%s::%s", md.Source, md.Sourcetype, md.Host))
	}
	e.jw.Encode(md)
}

func (e *Encoder) calcPartitionKey(md Metadata) string {
	if md.IsUnbroken {
		// unbroken streams (i.e. event boundaries are unknown) must use a hash
		// of the metadata so that all entries from the same source and encoder
		// run land on the same shard.
		return "unbroken-" + hashMetadata(md)
	}
	// use a random value; it doesn't matter which shard this lands on
	return "broken-" + strconv.FormatInt(mathrand.Int63(), 10)
}

// Decoder decodes the log data from a single Kinesis record,
// extracting the stream's metadata and any subsequent data entries.
type Decoder struct {
	jd       *json.Decoder
	gzr      *gzip.Reader
	metadata Metadata
}

// NewDecoder creates a new decoder from the compressed input data
// and extracts the metadata header.
func NewDecoder(data []byte) (*Decoder, error) {
	gzr, err := gzip.NewReader(bytes.NewReader(data))
	if err != nil {
		return nil, err
	}
	jd := json.NewDecoder(gzr)
	d := &Decoder{jd: jd, gzr: gzr}
	err = jd.Decode(&d.metadata)
	if err != nil {
		return nil, err
	}
	return d, nil
}

// Metadata returns the metadata decoded from the record header.
func (d *Decoder) Metadata() Metadata {
	return d.metadata
}

// Decode decodes the data for the next event entry
// found in the record, or returns io.EOF.
func (d *Decoder) Decode(buf *Data) error {
	return d.jd.Decode(buf)
}

func hashMetadata(md Metadata) string {
	h := md5.New()
	fmt.Fprintf(h, "%d::%s::%s::%s::%s",
		md.EncoderID, md.Index, md.Source, md.Sourcetype, md.Host)
	return fmt.Sprintf("%x", h.Sum(nil))
}
