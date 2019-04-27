// Copyright 2016 Anki, Inc.

package logrecord

import (
	"bytes"
	"compress/gzip"
	"encoding/json"
	"fmt"
	"io"
	"reflect"
	"testing"
	"time"
)

// Test that unique partition keys are correctly generated for broken sources
func TestBufferUnbrokenGenKey(t *testing.T) {
	seen := make(map[string]bool)

	testChanged := func(test string, md Metadata) {
		b := new(Encoder)
		b.Reset(md)
		pk := b.partitionKey
		if seen[pk] {
			t.Errorf("%s: duplicate partition key %s", test, pk)
		}

		b.Reset(md)
		newPk := b.partitionKey
		if newPk != pk {
			t.Errorf("%s: second reset did not return same partition key old=%s new=%s", test, pk, newPk)
		}
		seen[pk] = true
	}

	md := Metadata{
		IsUnbroken: true,
		Source:     "source1",
		Sourcetype: "sourcetype1",
		Host:       "host1",
	}

	for i := 0; i < 2; i++ {
		test := fmt.Sprintf("source%d", i)
		md.Source = test
		testChanged(test, md)

		test = fmt.Sprintf("sourcetype%d", i)
		md.Sourcetype = test
		testChanged(test, md)

		test = fmt.Sprintf("host%d", i)
		md.Host = test
		testChanged(test, md)
	}
}

// Test that a pre-broken stream of events generates a unique parittion key on each reset.
func TestBufferBrokenGenKey(t *testing.T) {
	md := Metadata{
		IsUnbroken: false,
		Source:     "source1",
		Sourcetype: "sourcetype1",
		Host:       "host1",
	}

	b := NewEncoder(md)
	pk := b.partitionKey

	b.Reset(md)
	newPk := b.partitionKey

	if pk == newPk {
		t.Errorf("pk=%s newPk=%s - should be different", pk, newPk)
	}
}

func TestBufferAddData(t *testing.T) {
	b := new(Encoder)

	tm := time.Date(2016, 1, 2, 3, 4, 5, 7, time.UTC)
	events := []Data{
		Data{IsDone: true, Time: &tm, Data: []byte("event 1")},
		Data{IsDone: false, Time: &tm, Data: []byte("event 2")},
	}

	md := Metadata{IsUnbroken: true, Source: "mysource", Sourcetype: "mysourcetype", Host: "myhost"}
	b.Reset(md)

	for i := 0; i < len(events); i++ {
		if err := b.Encode(events[i]); err != nil {
			t.Fatalf("addData (%d) failed: %s", i, err)
		}
	}

	raw := bytes.NewBuffer(b.Data())
	r, err := gzip.NewReader(raw)
	if err != nil {
		t.Fatal("gzip init failed", err)
	}
	dec := json.NewDecoder(r)

	var outmd Metadata
	if err := dec.Decode(&outmd); err != nil {
		t.Fatal("metadata read failed", err)
	}
	// set expected encoder id and block
	md.EncoderID = EncoderID
	md.BlockID = 1
	if !reflect.DeepEqual(md, outmd) {
		t.Errorf("metadata mismatch expected=%#v actual=%#v", md, outmd)
	}

	for i := 0; i < len(events); i++ {
		var outev Data
		if err := dec.Decode(&outev); err != nil {
			t.Fatalf("event read (%d) failed: %s", i, err)
		}
		if !reflect.DeepEqual(outev, events[i]) {
			t.Errorf("%d event mismatch expected=%#v actual=%#v", i, events[i], outev)
		}
	}

	// should be no more data
	var v interface{}
	if err := dec.Decode(&v); err == nil {
		t.Fatal("Unexpected data", v)
	}
}

func TestBufferReset(t *testing.T) {
	b := new(Encoder)
	md := Metadata{IsUnbroken: true, Source: "mysource", Sourcetype: "mysourcetype", Host: "myhost"}
	b.Reset(md)

	if err := b.Encode(Data{Data: []byte("event1")}); err != nil {
		t.Fatal("addData failed", err)
	}

	md2 := Metadata{IsUnbroken: true, Source: "mysource2", Sourcetype: "mysourcetype", Host: "myhost"}
	b.Reset(md2)
	if err := b.Encode(Data{Data: []byte("event2")}); err != nil {
		t.Fatal("addData failed", err)
	}

	r, err := gzip.NewReader(bytes.NewBuffer(b.Data()))
	if err != nil {
		t.Fatal("gzip init failed", err)
	}
	dec := json.NewDecoder(r)

	// should get md2 metadata
	var outmd Metadata
	md2.EncoderID = EncoderID
	md2.BlockID = 1
	if err := dec.Decode(&outmd); err != nil {
		t.Fatal("metadata read failed", err)
	}
	if !reflect.DeepEqual(md2, outmd) {
		t.Errorf("metadata mismatch expected=%#v actual=%#v", md2, outmd)
	}

	var outev Data
	if err := dec.Decode(&outev); err != nil {
		t.Fatalf("event read failed: %s", err)
	}
	if !bytes.Equal(outev.Data, []byte("event2")) {
		t.Errorf("event mismatch expected=%#q actual=%#q", "event2", string(outev.Data))
	}
}

// Sequential buffers with the same unbroken source/sourcetype/host should have
// sequential block ids
var seqTests = []struct {
	source          string
	expectedBlockID int64
}{
	{"source1", 1},
	{"source2", 1},
	{"source1", 2},
}

func TestBufferSequence(t *testing.T) {
	b := new(Encoder)

	for _, test := range seqTests {
		md := Metadata{IsUnbroken: true, Source: test.source, Sourcetype: "mysourcetype", Host: "myhost"}
		b.Reset(md)
		if err := b.Encode(Data{Data: []byte("event1")}); err != nil {
			t.Fatal("addData failed", err)
		}

		md.EncoderID = EncoderID
		md.BlockID = test.expectedBlockID
		r, err := gzip.NewReader(bytes.NewBuffer(b.Data()))
		if err != nil {
			t.Fatal("gzip init failed", err)
		}
		dec := json.NewDecoder(r)
		var outmd Metadata
		if err := dec.Decode(&outmd); err != nil {
			t.Fatal("metadata read failed", err)
		}
		if !reflect.DeepEqual(md, outmd) {
			t.Errorf("metadata mismatch test=%#v expected=%#v actual=%#v", test, md, outmd)
		}
	}
}

func TestBufferFull(t *testing.T) {
	defer func(size int) { MaxRecordSize = size }(MaxRecordSize)
	MaxRecordSize = 12

	b := new(Encoder)
	md := Metadata{IsUnbroken: true, Source: "mysource", Sourcetype: "mysourcetype", Host: "myhost"}
	b.Reset(md)

	// Add a couple of 6 byte entries
	if err := b.Encode(Data{Data: []byte("event1")}); err != nil {
		t.Fatal("Encode failed", err)
	}
	if err := b.Encode(Data{Data: []byte("event2")}); err != nil {
		t.Fatal("Encode failed", err)
	}

	// should now be at size limit
	if err := b.Encode(Data{Data: []byte("event3")}); err != ErrRecordTooLarge {
		t.Fatal("did not get RecordTooLarge error", err)
	}
}

func TestBufferNoReset(t *testing.T) {
	var ok bool
	func() {
		defer func() {
			if x := recover(); x != nil {
				fmt.Println("got panic", x)
				ok = true
			}
		}()
		b := new(Encoder)
		b.Encode(Data{Data: []byte("event1")})
	}()
	if !ok {
		t.Fatal("Encode did not trigger panic")
	}
}

func TestDeocder(t *testing.T) {
	encmd := Metadata{
		IsUnbroken: true,
		Source:     "mysource",
		Sourcetype: "mysourcetype",
		Host:       "myhost",
		Index:      "myindex",
	}

	now := time.Now()
	enc := NewEncoder(encmd)
	entries := []Data{
		Data{IsDone: false, Time: &now, Data: []byte("record1")},
		Data{IsDone: true, Time: &now, Data: []byte("record2")},
	}

	enc.Encode(entries[0])
	enc.Encode(entries[1])
	data := enc.Data()

	dec, err := NewDecoder(data)
	if err != nil {
		t.Fatal("Failed to initialize decoder", err)
	}
	md := dec.Metadata()
	if md.EncoderID != EncoderID {
		t.Errorf("Decoder metadata EncoderID incorrect expected=%d actual=%d",
			EncoderID, md.EncoderID)
	}

	md.BlockID = 0
	md.EncoderID = 0
	if !reflect.DeepEqual(encmd, md) {
		t.Errorf("Metadata mismatch expected=%#v actual=%#v", encmd, md)
	}

	var d Data
	for i := 0; i < len(entries); i++ {
		if err := dec.Decode(&d); err != nil {
			t.Fatalf("Failed to decode for i=%d error=%v", i, err)
		}
	}

	if err := dec.Decode(&d); err != io.EOF {
		t.Fatalf("Expected EOF got %#v", err)
	}
}

func TestDecodeError(t *testing.T) {
	if _, err := NewDecoder(nil); err == nil {
		t.Error("No error on bad gzip data")
	}

	var buf bytes.Buffer
	gz := gzip.NewWriter(&buf)
	gz.Write([]byte("invalid json"))
	gz.Close()

	if _, err := NewDecoder(buf.Bytes()); err == nil {
		t.Error("No error on bad json data")
	}
}
