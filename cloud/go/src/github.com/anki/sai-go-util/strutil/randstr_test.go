// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package strutil

import (
	"errors"
	"io"
	"testing"
)

type NumReader int

func (nr NumReader) Read(buf []byte) (n int, err error) {
	for i := 0; i < cap(buf); i++ {
		buf[i] = byte(i + int(nr))
	}
	return cap(buf), nil
}

type errReader struct{ err error }

func (e errReader) Read(p []byte) (n int, err error) {
	return 0, e.err
}

// Test the reader by generating an artifical random number source that starts at 2
// Will cause the reader to start 2 characters into the charset and wrap around at the end
func TestReader(t *testing.T) {
	orgReader := RandSource
	defer func() { RandSource = orgReader }()
	RandSource = NumReader(2)

	buf := make([]byte, len(AllSafe))
	n, err := AllSafe.Read(buf)
	if err != nil {
		t.Fatalf("Read failure err=%v", err)
	}
	if n != len(AllSafe) {
		t.Errorf("Read count is incorrect expected=%d actual=%d", len(AllSafe), n)
	}
	expected := string(AllSafe[2:]) + string(AllSafe[0:2])
	actual := string(buf)
	if actual != expected {
		t.Errorf("Didn't get expected result expected=%q actual=%q", expected, actual)
	}
}

var utests = []struct {
	name     string
	f        func() (string, error)
	expected string
}{
	{"lower", ShortLowerUUID, "456789abcdefghjkmnpqrstuvw"},
	{"mixed", ShortMixedUUID, "456789abcdefghjkmnpqrst"},
}

func TestShortUUID(t *testing.T) {
	orgReader := RandSource
	defer func() { RandSource = orgReader }()

	for _, tst := range utests {
		RandSource = NumReader(2)
		result, err := tst.f()
		if err != nil {
			t.Errorf("Unexpected error f=%s error=%q", tst.name, err)
		} else if result != tst.expected {
			t.Errorf("Unexpected error f=%s expected=%q actual=%q", tst.name, tst.expected, result)
		}
	}
}

func TestNewReader(t *testing.T) {
	var r Reader = NewReader("abc").(Reader)
	if string(r) != "abc" {
		t.Errorf("Reader does not contain requestsd character set")
	}
}

func TestReadErrors(t *testing.T) {
	orgReader := RandSource
	defer func() { RandSource = orgReader }()
	RandSource = errReader{errors.New("Read Failure")}

	for _, f := range []func() (string, error){ShortLowerUUID, ShortMixedUUID} {
		_, err := f()
		if err == nil {
			t.Error("Didn't get expected error")
		}
	}
}

// Test that using the actual rand.Random() works ok!
func TestRandom(t *testing.T) {
	result, err := ShortMixedUUID()
	if err != nil {
		t.Fatal("Unxpected error from ShortMixedUUID", err)
	}
	if len(result) != 23 {
		t.Errorf("Unxpected result length from ShortMixedUUID expected=18 actual=%d", len(result))
	}

	// It'd be pretty unlikely that all the bytes are the same!
	same := 0
	for i := 1; i < len(result); i++ {
		if result[0] == result[i] {
			same++
		}
	}
	if same == 17 {
		t.Errorf("Resulting bytes are all identical!  result=%q", result)
	}
}

func TestStr(t *testing.T) {
	expected := "aaaaa"
	result, err := Str(Reader("a"), 5)
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	if result != expected {
		t.Errorf("result=%q expected=%q", result, expected)
	}
}

func TestLowerStr(t *testing.T) {
	defer func(orgReader io.Reader) { RandSource = orgReader }(RandSource)
	RandSource = NumReader(2)

	result := LowerStr(5)
	expected := "45678"
	if result != expected {
		t.Errorf("result=%q expected=%q", result, expected)
	}
}

func BenchmarkMixedWithRandom(b *testing.B) {
	for i := 0; i < b.N; i++ {
		_, err := ShortMixedUUID()
		if err != nil {
			b.Fatal("Unexpected error", err)
		}
	}
}

func BenchmarkMixedNoRandom(b *testing.B) {
	orgReader := RandSource
	defer func() { RandSource = orgReader }()
	RandSource = NumReader(2)

	for i := 0; i < b.N; i++ {
		_, err := ShortMixedUUID()
		if err != nil {
			b.Fatal("Unexpected error", err)
		}
	}
}
