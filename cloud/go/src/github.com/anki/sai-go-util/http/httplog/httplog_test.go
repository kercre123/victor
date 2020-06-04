// Copyright 2013 The Gorilla Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package httplog

import (
	"bytes"
	"net"
	"net/http"
	"net/url"
	"strings"
	"testing"
	"time"
)

const (
	ok = "ok\n"
)

var okHandler = http.HandlerFunc(func(w http.ResponseWriter, req *http.Request) {
	w.Write([]byte(ok))
})

func newRequest(method, url string) *http.Request {
	req, err := http.NewRequest(method, url, nil)
	if err != nil {
		panic(err)
	}
	return req
}

type strAppend string

func (s strAppend) AppendToLog(buf []byte, req *http.Request, ts time.Time, status int, size int) []byte {
	return append(buf, s...)
}

type strAppendWithSize struct {
	retval     string
	sizecalled bool
}

func (s strAppendWithSize) AppendToLog(buf []byte, req *http.Request, ts time.Time, status int, size int) []byte {
	return append(buf, s.retval...)
}

func (s *strAppendWithSize) EstimateAppendSize(req *http.Request, ts time.Time, status int, size int) int {
	s.sizecalled = true
	return len(s.retval)
}

func TestWriteLog(t *testing.T) {
	loc, err := time.LoadLocation("Europe/Warsaw")
	if err != nil {
		panic(err)
	}
	ts := time.Date(1983, 05, 26, 3, 30, 45, 0, loc)

	// A typical request with an OK response
	req := newRequest("GET", "http://example.com")
	req.RemoteAddr = "192.168.100.5"

	buf := new(bytes.Buffer)
	h := LogHandler{writer: buf}
	h.writeLog(buf, req, ts, http.StatusOK, 100)
	log := buf.String()

	expected := "192.168.100.5 - - [26/May/1983:03:30:45 +0200] \"GET / HTTP/1.1\" 200 100\n"
	if log != expected {
		t.Fatalf("wrong log, got %q want %q", log, expected)
	}

	// Request with an unauthorized user
	req = newRequest("GET", "http://example.com")
	req.RemoteAddr = "192.168.100.5"
	req.URL.User = url.User("kamil")

	buf.Reset()
	h.writeLog(buf, req, ts, http.StatusUnauthorized, 500)
	log = buf.String()

	expected = "192.168.100.5 - kamil [26/May/1983:03:30:45 +0200] \"GET / HTTP/1.1\" 401 500\n"
	if log != expected {
		t.Fatalf("wrong log, got %q want %q", log, expected)
	}

	// Request with url encoded parameters
	req = newRequest("GET", "http://example.com/test?abc=hello%20world&a=b%3F")
	req.RemoteAddr = "192.168.100.5"

	buf.Reset()
	h.writeLog(buf, req, ts, http.StatusOK, 100)
	log = buf.String()

	expected = "192.168.100.5 - - [26/May/1983:03:30:45 +0200] \"GET /test?abc=hello%20world&a=b%3F HTTP/1.1\" 200 100\n"
	if log != expected {
		t.Fatalf("wrong log, got %q want %q", log, expected)
	}
}

func TestWriteCombinedLog(t *testing.T) {
	loc, err := time.LoadLocation("Europe/Warsaw")
	if err != nil {
		panic(err)
	}
	ts := time.Date(1983, 05, 26, 3, 30, 45, 0, loc)

	// A typical request with an OK response
	req := newRequest("GET", "http://example.com")
	req.RemoteAddr = "192.168.100.5"
	req.Header.Set("Referer", "http://example.com")
	req.Header.Set(
		"User-Agent",
		"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) AppleWebKit/537.33 "+
			"(KHTML, like Gecko) Chrome/27.0.1430.0 Safari/537.33",
	)

	buf := new(bytes.Buffer)
	h := LogHandler{writer: buf, isCombined: true}
	h.writeLog(buf, req, ts, http.StatusOK, 100)
	log := buf.String()

	expected := "192.168.100.5 - - [26/May/1983:03:30:45 +0200] \"GET / HTTP/1.1\" 200 100 \"http://example.com\" " +
		"\"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) " +
		"AppleWebKit/537.33 (KHTML, like Gecko) Chrome/27.0.1430.0 Safari/537.33\"\n"
	if log != expected {
		t.Fatalf("wrong log, got %q want %q", log, expected)
	}

	// Request with an unauthorized user
	req.URL.User = url.User("kamil")

	buf.Reset()
	h.writeLog(buf, req, ts, http.StatusUnauthorized, 500)
	log = buf.String()

	expected = "192.168.100.5 - kamil [26/May/1983:03:30:45 +0200] \"GET / HTTP/1.1\" 401 500 \"http://example.com\" " +
		"\"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) " +
		"AppleWebKit/537.33 (KHTML, like Gecko) Chrome/27.0.1430.0 Safari/537.33\"\n"
	if log != expected {
		t.Fatalf("wrong log, got %q want %q", log, expected)
	}

	// Test with remote ipv6 address
	req.RemoteAddr = "::1"

	buf.Reset()
	h.writeLog(buf, req, ts, http.StatusOK, 100)
	log = buf.String()

	expected = "::1 - kamil [26/May/1983:03:30:45 +0200] \"GET / HTTP/1.1\" 200 100 \"http://example.com\" " +
		"\"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) " +
		"AppleWebKit/537.33 (KHTML, like Gecko) Chrome/27.0.1430.0 Safari/537.33\"\n"
	if log != expected {
		t.Fatalf("wrong log, got %q want %q", log, expected)
	}

	// Test remote ipv6 addr, with port
	req.RemoteAddr = net.JoinHostPort("::1", "65000")

	buf.Reset()
	h.writeLog(buf, req, ts, http.StatusOK, 100)
	log = buf.String()

	expected = "::1 - kamil [26/May/1983:03:30:45 +0200] \"GET / HTTP/1.1\" 200 100 \"http://example.com\" " +
		"\"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) " +
		"AppleWebKit/537.33 (KHTML, like Gecko) Chrome/27.0.1430.0 Safari/537.33\"\n"
	if log != expected {
		t.Fatalf("wrong log, got %q want %q", log, expected)
	}
}

var remoteHostTests = []struct {
	reqAddr          string
	xFwdAddr         string
	expectedEnabled  string
	expectedDisabled string
}{
	{"1.1.1.1:10000", "", "1.1.1.1", "1.1.1.1"},
	{"1.1.1.1", "", "1.1.1.1", "1.1.1.1"},
	{"1.1.1.1", "2.2.2.2,3.3.3.3", "3.3.3.3", "1.1.1.1"},
	{"1.1.1.1", "2.2.2.2, 4.4.4.4 ", "4.4.4.4", "1.1.1.1"},
	{"1.1.1.1:10000", "2.2.2.2:8000", "2.2.2.2", "1.1.1.1"},
	{"[::1]:10000", "[::2]:10000", "::2", "::1"},
}

func TestRemoteHost(t *testing.T) {
	hFF := LogHandler{UseForwardedFor: true}
	hNoFF := LogHandler{UseForwardedFor: false}

	for _, test := range remoteHostTests {
		req, _ := http.NewRequest("GET", "", nil)
		req.RemoteAddr = test.reqAddr
		if test.xFwdAddr != "" {
			req.Header.Set(xForwardedFor, test.xFwdAddr)
		}

		withFFResult := hFF.remoteHost(req)
		if withFFResult != test.expectedEnabled {
			t.Errorf("mismatch withFF reqAddr=%q xFwdAddr=%q expected=%q actual=%q",
				test.reqAddr, test.xFwdAddr, test.expectedEnabled, withFFResult)
		}

		noFFResult := hNoFF.remoteHost(req)
		if noFFResult != test.expectedDisabled {
			t.Errorf("mismatch noFF reqAddr=%q xFwdAddr=%q expected=%q actual=%q",
				test.reqAddr, test.xFwdAddr, test.expectedDisabled, noFFResult)
		}
	}
}

func TestSuffixFormatter(t *testing.T) {
	loc, err := time.LoadLocation("Europe/Warsaw")
	if err != nil {
		panic(err)
	}
	ts := time.Date(1983, 05, 26, 3, 30, 45, 0, loc)

	// A typical request with an OK response
	req := newRequest("GET", "http://example.com")
	req.RemoteAddr = "192.168.100.5"

	buf := new(bytes.Buffer)
	h := LogHandler{writer: buf}
	appender := strAppend("Extra data")
	h.SuffixFormatter = appender

	h.writeLog(buf, req, ts, http.StatusOK, 100)
	log := buf.String()

	expected := "192.168.100.5 - - [26/May/1983:03:30:45 +0200] \"GET / HTTP/1.1\" 200 100 Extra data\n"
	if log != expected {
		t.Fatalf("wrong log, got %q want %q", log, expected)
	}

	sizeAppender := &strAppendWithSize{"Another value", false}
	h.SuffixFormatter = sizeAppender
	buf.Reset()

	h.writeLog(buf, req, ts, http.StatusOK, 100)
	log = buf.String()

	expected = "192.168.100.5 - - [26/May/1983:03:30:45 +0200] \"GET / HTTP/1.1\" 200 100 Another value\n"
	if log != expected {
		t.Fatalf("wrong log, got %q want %q", log, expected)
	}

	if !sizeAppender.sizecalled {
		t.Errorf("EstimateAppendSize was not called")
	}
}

func BenchmarkWriteLog(b *testing.B) {
	loc, err := time.LoadLocation("Europe/Warsaw")
	if err != nil {
		b.Fatalf(err.Error())
	}
	ts := time.Date(1983, 05, 26, 3, 30, 45, 0, loc)

	req := newRequest("GET", "http://example.com")
	req.RemoteAddr = "192.168.100.5"

	b.ResetTimer()

	buf := &bytes.Buffer{}
	h := LogHandler{writer: buf}
	for i := 0; i < b.N; i++ {
		buf.Reset()
		h.writeLog(buf, req, ts, http.StatusUnauthorized, 500)
	}
}

func BenchmarkWriteCombinedLog(b *testing.B) {
	loc, err := time.LoadLocation("Europe/Warsaw")
	if err != nil {
		b.Fatalf(err.Error())
	}
	ts := time.Date(1983, 05, 26, 3, 30, 45, 0, loc)

	req := newRequest("GET", "http://example.com")
	req.RemoteAddr = "192.168.100.5"
	req.Header.Set("Referer", "http://example.com")
	req.Header.Set(
		"User-Agent",
		"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) AppleWebKit/537.33 "+
			"(KHTML, like Gecko) Chrome/27.0.1430.0 Safari/537.33",
	)

	b.ResetTimer()

	buf := &bytes.Buffer{}
	h := LogHandler{writer: buf}
	for i := 0; i < b.N; i++ {
		buf.Reset()
		h.writeLog(buf, req, ts, http.StatusUnauthorized, 500)
	}
}

func BenchmarkSuffixLog(b *testing.B) {
	loc, err := time.LoadLocation("Europe/Warsaw")
	if err != nil {
		b.Fatalf(err.Error())
	}
	ts := time.Date(1983, 05, 26, 3, 30, 45, 0, loc)

	req := newRequest("GET", "http://example.com")
	req.RemoteAddr = "192.168.100.5"

	b.ResetTimer()

	buf := &bytes.Buffer{}
	h := LogHandler{writer: buf}
	appender := strAppend("Extra data")
	h.SuffixFormatter = appender

	for i := 0; i < b.N; i++ {
		buf.Reset()
		h.writeLog(buf, req, ts, http.StatusUnauthorized, 500)
	}
}

func BenchmarkSuffixLogNoSize(b *testing.B) {
	loc, err := time.LoadLocation("Europe/Warsaw")
	if err != nil {
		b.Fatalf(err.Error())
	}
	ts := time.Date(1983, 05, 26, 3, 30, 45, 0, loc)

	req := newRequest("GET", "http://example.com")
	req.RemoteAddr = "192.168.100.5"

	b.ResetTimer()

	buf := &bytes.Buffer{}
	h := LogHandler{writer: buf}
	appender := strAppend(strings.Repeat("String longer than 50 bytes", 5))
	h.SuffixFormatter = appender

	for i := 0; i < b.N; i++ {
		buf.Reset()
		h.writeLog(buf, req, ts, http.StatusUnauthorized, 500)
	}
}

func BenchmarkSuffixLogWithSize(b *testing.B) {
	loc, err := time.LoadLocation("Europe/Warsaw")
	if err != nil {
		b.Fatalf(err.Error())
	}
	ts := time.Date(1983, 05, 26, 3, 30, 45, 0, loc)

	req := newRequest("GET", "http://example.com")
	req.RemoteAddr = "192.168.100.5"

	b.ResetTimer()

	buf := &bytes.Buffer{}
	h := LogHandler{writer: buf}
	appender := &strAppendWithSize{strings.Repeat("String longer than 50 bytes", 5), false}
	h.SuffixFormatter = appender

	for i := 0; i < b.N; i++ {
		buf.Reset()
		h.writeLog(buf, req, ts, http.StatusUnauthorized, 500)
	}
	if !appender.sizecalled {
		b.Errorf("size wasn't called!")
	}
}
