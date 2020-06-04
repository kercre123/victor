// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package strutil

import (
	"reflect"
	"strings"
	"testing"
)

var encodeTests = []struct {
	reader   Reader
	length   int
	n        uint64
	expected string
}{
	{Reader("01"), 0, 42, "101010"},
	{Reader("ab"), 8, 42, "aabababa"},
}

func TestEncodeStr(t *testing.T) {
	for _, test := range encodeTests {
		result := EncodeInt(test.reader, test.length, test.n)
		if result != test.expected {
			t.Errorf("test=%#v expected=%q actual=%q", test, test.expected, result)
		}
	}
}

var toBinTests = []struct {
	reader   Reader
	input    string
	expected []byte
}{
	{"0123456789", "16", []byte{16}},
	{"0123456789", "260", []byte{1, 4}},
	{"0123456789abcdef", "104", []byte{1, 4}},
	{AllSafe, strings.Repeat("Z", 23), []byte{0x14, 0x91, 0xb5, 0xf1, 0x52, 0x98, 0x5, 0x4d, 0xc5, 0x2b, 0xa9, 0xa6, 0xb3, 0xf9, 0x7f, 0xff, 0xff}},
}

func TestToBinary(t *testing.T) {
	for _, test := range toBinTests {
		result := ToBinary(test.reader, test.input)
		if !reflect.DeepEqual(test.expected, result) {
			t.Errorf("Mismatch for reader=%q input=%q expected=%x actual=%#v", test.reader, test.input, test.expected, result)
		}
	}
}

var fromBinTests = []struct {
	reader    Reader
	input     []byte
	minLength int
	expected  string
}{
	{"0123456789", []byte{17}, 0, "17"},
	{"123456789", []byte{17}, 0, "29"},
	{"123456789", []byte{17}, 5, "11129"},
	{AllSafe, []byte{0x14, 0x91, 0xb5, 0xf1, 0x52, 0x98, 0x5, 0x4d, 0xc5, 0x2b, 0xa9, 0xa6, 0xb3, 0xf9, 0x7f, 0xff, 0xff}, 0, strings.Repeat("Z", 23)},
}

func TestFromBinary(t *testing.T) {
	for _, test := range fromBinTests {
		result := FromBinary(test.reader, test.input, test.minLength)
		if result != test.expected {
			t.Errorf("Mismatch for reader=%q input=%x expected=%q actual=%q", test.reader, test.input, test.expected, result)
		}
	}
}

var toUUIDTests = []struct {
	reader   Reader
	input    string
	expected string
}{
	{"0123456789abcdef", "deadbeef987654321", "00000000-0000-000d-eadb-eef987654321"},
	{"0123456789abcdef", "123456781234123deadbeef987654321", "12345678-1234-123d-eadb-eef987654321"},
}

func TestToUUIDFormat(t *testing.T) {
	for _, test := range toUUIDTests {
		result, err := ToUUIDFormat(test.reader, test.input)
		if err != nil {
			t.Errorf("Unexpected error for reader=%q input=%q err=%v", test.reader, test.input, err)
		}
		if result != test.expected {
			t.Errorf("Mismatch for  reader=%q input=%x expected=%q actual=%q", test.reader, test.input, test.expected, result)
		}
	}
}

var fromUUIDTests = []struct {
	reader    Reader
	input     string
	minLength int
	expected  string
}{
	{"0123456789abcdef", "00000000-0000-000D-EADB-EEF987654321", 0, "deadbeef987654321"},
	{"0123456789abcdef", "00000000-0000-000D-EADB-EEF987654321", 32, "000000000000000deadbeef987654321"},
	{"0123456789abcdef", "12345678-1234-123d-eadb-eef987654321", 0, "123456781234123deadbeef987654321"},
}

func TestFromUUIDFormat(t *testing.T) {
	for _, test := range fromUUIDTests {
		result, err := FromUUIDFormat(test.reader, test.input, test.minLength)
		if err != nil {
			t.Errorf("Unexpected error for reader=%q input=%q err=%v", test.reader, test.input, err)
		}
		if result != test.expected {
			t.Errorf("Mismatch for  reader=%q input=%q expected=%q actual=%q", test.reader, test.input, test.expected, result)
		}
	}
}

func BenchmarkToBinary(b *testing.B) {
	reader := AllSafe
	input := strings.Repeat("Z", 23)
	for i := 0; i < b.N; i++ {
		ToBinary(reader, input)
	}
}

func BenchmarkToUUID(b *testing.B) {
	reader := AllSafe
	input := strings.Repeat("Z", 23)
	for i := 0; i < b.N; i++ {
		ToUUIDFormat(reader, input)
	}
}

func BenchmarkFromUUID(b *testing.B) {
	reader := AllSafe
	input := "12345678-1234-123D-EADB-EEF987654321"
	for i := 0; i < b.N; i++ {
		FromUUIDFormat(reader, input, 0)
	}
}
