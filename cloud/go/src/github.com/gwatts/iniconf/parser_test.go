// Copyright Splunk, Inc. 2014
//
// Author: Gareth Watts <gareth@splunk.com>

package iniconf

import (
	"bufio"
	"fmt"
	"io"
	"strings"
	"testing"
)

var singleEntryTests = []struct {
	name                string
	input               string
	expectedType        int
	expectedLineCount   int
	expectedLineLengths []int
}{
	{"blank", "\n", entryTypeBlank, 1, []int{0}},
	{"comment", "; a comment\n", entryTypeComment, 1, []int{len("; a comment")}},
	{"section", "[a section]\n", entryTypeSection, 1, []int{len("[a section]")}},
	{"kv", "a key=a val\n", entryTypeKV, 1, []int{len("a key=a val")}},
	{"kv_2line", "a key=val line1\n val line2", entryTypeKV, 2, []int{len("a key=val line1"), len(" val line2")}},
}

func TestParseSingleValue(t *testing.T) {
	for _, test := range singleEntryTests {
		br := bufio.NewReader(strings.NewReader(test.input))
		p := newParser(br)
		entry, err := p.NextValue()
		if err != nil {
			t.Errorf("test=%s received err=%s", test.name, err)
			continue
		}
		if entry.entryType != test.expectedType {
			t.Errorf("test=%s entryType mismatch expected=%d actual=%d", test.name, test.expectedType, entry.entryType)
		}
		if len(entry.rawLines) != test.expectedLineCount {
			t.Errorf("test=%s linecount mismatch expected=%d actual=%d", test.name, test.expectedLineCount, len(entry.rawLines))
		}
		for i := 0; i < len(entry.rawLines); i++ {
			if len(entry.rawLines[i]) != test.expectedLineLengths[i] {
				t.Errorf("test=%s line length mismatch for linenum=%d line=%q", test.name, i, string(entry.rawLines[i]))
			}
		}

		// next read should return an EOF error
		entry, err = p.NextValue()
		if err != io.EOF {
			t.Fatalf("test=%s expected err=EOF, actual=%s", test.name, err)
		}
		if entry.entryType != entryTypeNone {
			t.Errorf("test=%s expected entry=nil actual=%d", test.name, entry.entryType)
		}

	}
}

// Test reading a multiline entry followed by a single line entry
func TestParseMultiValue(t *testing.T) {
	input := "key1=line one\n line two\nkey2=line three\n"
	br := bufio.NewReader(strings.NewReader(input))
	p := newParser(br)
	entry, err := p.NextValue()
	if err != nil {
		t.Fatalf("read 1 received err=%s", err)
	}
	if entry.key != "key1" {
		t.Errorf("key mismatch expected=key1 actual=%s", entry.key)
	}
	if string(entry.value) != "line one\nline two" {
		t.Errorf("val mismatch expected=\"line one\nline two\" actual=%q", entry.value)
	}

	entry, err = p.NextValue()
	if err != nil {
		t.Fatalf("read 2 received err=%s", err)
	}
	if entry.key != "key2" {
		t.Errorf("key mismatch expected=key2 actual=%s", entry.key)
	}
	if string(entry.value) != "line three" {
		t.Errorf("val mismatch expected=\"line three\" actual=%q", entry.value)
	}

	entry, err = p.NextValue()
	if err != io.EOF {
		t.Errorf("expected err=EOF, actual=%s", err)
	}
}

// Make sure that overly long lines generate an error
func TestLongLineErr(t *testing.T) {
	// create a reader with a buffer size of only 10 characters
	// lines longer than that will be error
	input := "key1=an entry longer than 10 chars"
	br := bufio.NewReaderSize(strings.NewReader(input), 10)
	p := newParser(br)
	_, err := p.NextValue()
	if err.(*ParseError).Code() != ErrLineTooLong {
		t.Fatalf("expected err=ErrLineTooLong actual=%s", err)
	}
}

// Make sure that invalid section definitions generate an error
func TestBadSectionDef(t *testing.T) {
	input := "[section\n"
	br := bufio.NewReader(strings.NewReader(input))
	p := newParser(br)
	_, err := p.NextValue()
	if err.(*ParseError).Code() != ErrInvalidSection {
		t.Fatalf("expected err=ErrInvalidSection actual=%s", err)
	}
}

// Make sure that lines that don't have an = statement generate an error
func TestBadKVDef(t *testing.T) {
	input := "badentry"
	br := bufio.NewReader(strings.NewReader(input))
	p := newParser(br)
	_, err := p.NextValue()
	if err.(*ParseError).Code() != ErrInvalidEntry {
		t.Fatalf("expected err=ErrInvalidEntry actual=%s", err)
	}
}

func TestStartsWithSpaceError(t *testing.T) {
	// supply an invalid utf8 byte
	if startsWithSpace([]byte{0xff, 0xff}) {
		t.Errorf("Failed to detect bad utf8 char")
	}
}

func TestParserError(t *testing.T) {
	p := newParseError(ErrInvalidSection, 123)
	if p.LineNum() != 123 {
		t.Errorf("Invalid line number expected=123 actual=%d", p.LineNum())
	}
	if p.Code() != ErrInvalidSection {
		t.Errorf("Invalid msg expected=%q actual=%q", ErrInvalidSection, p.Msg())
	}
	expected := fmt.Sprintf("%s at line %d", p.Msg(), p.LineNum())
	if p.Error() != expected {
		t.Errorf("Invalid error msg expected=%q actual=%q", expected, p.Error())
	}
}
