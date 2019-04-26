// Copyright 2016 Anki, Inc.

package alog

import "testing"

var nlEscapeTests = []struct {
	input    string
	expected string
}{
	{"simple", "simple"},
	{"one\ntwo", "one\\ntwo"},
	{"one\rtwo", "one\\rtwo"},
	{"one\ntwo\nthree\n", "one\\ntwo\\nthree\\n"},
	{"one\r\ntwo\r\nthree\r\n", "one\\r\\ntwo\\r\\nthree\\r\\n"},
}

func TestEscapeNewline(t *testing.T) {
	for _, test := range nlEscapeTests {
		result := string(escapeNewlines([]byte(test.input)))

		if result != test.expected {
			t.Errorf("Mismatch input=%q expected=%q actual=%q", test.input, test.expected, result)
		}
	}
}

func BenchmarkFmtCaller(b *testing.B) {
	buf := make([]byte, 0, 100)
	for i := 0; i < b.N; i++ {
		fmtCaller(&buf, 1)
	}
}
