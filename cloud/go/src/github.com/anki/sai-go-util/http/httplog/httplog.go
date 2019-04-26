// Copyright 2013 The Gorilla Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// httplog is a fork of the github.com/gorilla/handlers http logging
// library.
//
// This version:
// * Adds support for user-defined log suffixes,
// * Adds support for the X-Forwarded-For header
// * Improves memory use in the case of combined access logs
package httplog

import (
	"bufio"
	"io"
	"net"
	"net/http"
	"strconv"
	"strings"
	"time"
	"unicode/utf8"
)

const (
	xForwardedFor = "X-Forwarded-For"
)

var (
	// If a SuffixFormatter does not implement the SizeEstimate interface then
	// the logger will assume the append operation will return DefaultAppendSize bytes
	DefaultAppendSize = 50
)

// Types implementing the LogFormatter interface can be used as a SuffixFormatter
type LogFormatter interface {
	// AppendToLog takes a partially finished log line in buf and appends more data to it
	// before returning the line.  It should not append a newline.
	// ts is the timestamp with which the entry should be logged.
	// status and size are used to provide the response HTTP status and size.
	AppendToLog(buf []byte, req *http.Request, ts time.Time, status int, size int) []byte
}

// Types implementing the SizeEstimate interface can be used as a SuffixFormatter.
// Implementation of this interface by SuffixFormatters is optional, but can increase
// performance and reduce memory usage by allowing the LogHandler to accurately
// pre-allocate memory for the log entry prior to formatting it.
type SizeEstimate interface {
	// EstimateAppendSize returns the number of bytes AppendToLog is predicted to add
	// to the log.
	EstimateAppendSize(req *http.Request, ts time.Time, status int, size int) int
}

// LogHandler wraps an http.Handler and io.Writer and writes an access log
// to the writer recording the actvity of the wrapped handler.
type LogHandler struct {
	writer     io.Writer
	handler    http.Handler
	isCombined bool

	// SuffixFormatter specifies an optional LogFormatter that appends data
	// to a logline.  The formatter may also implement the SizeEstimate interface
	// to improve performance around memory allocations.
	SuffixFormatter LogFormatter

	// Set UseForwardedFor to true to have the LogHandler use the remote IP address
	// located in the X-Forwarded-For header instead of the request's RemoteAddr.
	// Will fallback to RemoteAddr if not set.
	UseForwardedFor bool
}

func (h LogHandler) ServeHTTP(w http.ResponseWriter, req *http.Request) {
	t := time.Now()
	var logger loggingResponseWriter
	if _, ok := w.(http.Hijacker); ok {
		logger = &hijackLogger{responseLogger: responseLogger{w: w}}
	} else {
		logger = &responseLogger{w: w}
	}
	h.handler.ServeHTTP(logger, req)
	h.writeLog(h.writer, req, t, logger.Status(), logger.Size())
}

func (h *LogHandler) remoteHost(req *http.Request) (host string) {
	var (
		addr string
		err  error
	)
	if h.UseForwardedFor {
		if addr = req.Header.Get(xForwardedFor); addr != "" {
			if i := strings.LastIndex(addr, ","); i > -1 {
				// get the last proxy in the chain, if more than one
				// the last one is the *only* reliable address, and even
				// then its only reliable if you know your proxy will always
				// set the header.
				addr = trim(addr[i+1:])
			}
		} else {
			addr = req.RemoteAddr
		}
	} else {
		addr = req.RemoteAddr
	}
	host, _, err = net.SplitHostPort(addr)
	if err != nil {
		host = addr
	}
	return host
}

func trim(s string) string {
	for len(s) > 0 && s[0] == ' ' {
		s = s[1:]
	}
	for len(s) > 0 && s[len(s)-1] == ' ' {
		s = s[:len(s)-1]
	}
	return s
}

type loggingResponseWriter interface {
	http.ResponseWriter
	Status() int
	Size() int
}

// responseLogger is wrapper of http.ResponseWriter that keeps track of its HTTP status
// code and body size
type responseLogger struct {
	w      http.ResponseWriter
	status int
	size   int
}

func (l *responseLogger) Header() http.Header {
	return l.w.Header()
}

func (l *responseLogger) Write(b []byte) (int, error) {
	if l.status == 0 {
		// The status will be StatusOK if WriteHeader has not been called yet
		l.status = http.StatusOK
	}
	size, err := l.w.Write(b)
	l.size += size
	return size, err
}

func (l *responseLogger) WriteHeader(s int) {
	l.w.WriteHeader(s)
	l.status = s
}

func (l *responseLogger) Status() int {
	return l.status
}

func (l *responseLogger) Size() int {
	return l.size
}

type hijackLogger struct {
	responseLogger
}

func (l *hijackLogger) Hijack() (net.Conn, *bufio.ReadWriter, error) {
	h := l.responseLogger.w.(http.Hijacker)
	conn, rw, err := h.Hijack()
	if err == nil && l.responseLogger.status == 0 {
		// The status will be StatusSwitchingProtocols if there was no error and WriteHeader has not been called yet
		l.responseLogger.status = http.StatusSwitchingProtocols
	}
	return conn, rw, err
}

const lowerhex = "0123456789abcdef"

// AppendQuoted adds quote marks around a string and escapes it so that its suitable
// to include in the access log and appens the string to the log entry supplied in buf.
func AppendQuoted(buf []byte, s string) []byte {
	var runeTmp [utf8.UTFMax]byte
	for width := 0; len(s) > 0; s = s[width:] {
		r := rune(s[0])
		width = 1
		if r >= utf8.RuneSelf {
			r, width = utf8.DecodeRuneInString(s)
		}
		if width == 1 && r == utf8.RuneError {
			buf = append(buf, `\x`...)
			buf = append(buf, lowerhex[s[0]>>4])
			buf = append(buf, lowerhex[s[0]&0xF])
			continue
		}
		if r == rune('"') || r == '\\' { // always backslashed
			buf = append(buf, '\\')
			buf = append(buf, byte(r))
			continue
		}
		if strconv.IsPrint(r) {
			n := utf8.EncodeRune(runeTmp[:], r)
			buf = append(buf, runeTmp[:n]...)
			continue
		}
		switch r {
		case '\a':
			buf = append(buf, `\a`...)
		case '\b':
			buf = append(buf, `\b`...)
		case '\f':
			buf = append(buf, `\f`...)
		case '\n':
			buf = append(buf, `\n`...)
		case '\r':
			buf = append(buf, `\r`...)
		case '\t':
			buf = append(buf, `\t`...)
		case '\v':
			buf = append(buf, `\v`...)
		default:
			switch {
			case r < ' ':
				buf = append(buf, `\x`...)
				buf = append(buf, lowerhex[s[0]>>4])
				buf = append(buf, lowerhex[s[0]&0xF])
			case r > utf8.MaxRune:
				r = 0xFFFD
				fallthrough
			case r < 0x10000:
				buf = append(buf, `\u`...)
				for s := 12; s >= 0; s -= 4 {
					buf = append(buf, lowerhex[r>>uint(s)&0xF])
				}
			default:
				buf = append(buf, `\U`...)
				for s := 28; s >= 0; s -= 4 {
					buf = append(buf, lowerhex[r>>uint(s)&0xF])
				}
			}
		}
	}
	return buf

}

type logEntry struct {
	req      *http.Request
	username string
	host     string
	uri      string
	ts       time.Time
	status   int
	size     int
	referer  string
	ua       string
}

func (ce *logEntry) estimateCommonSize() int {
	return 3 * (len(ce.host) + len(ce.username) + len(ce.req.Method) + len(ce.uri) + len(ce.req.Proto) + 50) / 2
}

// appendCommonToBuf builds a log entry for req in Apache Common Log Format.
// ts is the timestamp with which the entry should be logged.
// status and size are used to provide the response HTTP status and size.
func (ce *logEntry) appendCommonToBuf(buf []byte) []byte {
	buf = append(buf, ce.host...)
	buf = append(buf, " - "...)
	buf = append(buf, ce.username...)
	buf = append(buf, " ["...)
	buf = append(buf, ce.ts.Format("02/Jan/2006:15:04:05 -0700")...)
	buf = append(buf, `] "`...)
	buf = append(buf, ce.req.Method...)
	buf = append(buf, " "...)
	buf = AppendQuoted(buf, ce.uri)
	buf = append(buf, " "...)
	buf = append(buf, ce.req.Proto...)
	buf = append(buf, `" `...)
	buf = append(buf, strconv.Itoa(ce.status)...)
	buf = append(buf, " "...)
	buf = append(buf, strconv.Itoa(ce.size)...)
	return buf
}

func (ce *logEntry) estimateCombinedSize() int {
	return 3 * (len(ce.referer) + len(ce.ua) + 50) / 2
}

// appendCombinedToBuf writes a log entry for req to w in Apache Combined Log Format.
// ts is the timestamp with which the entry should be logged.
// status and size are used to provide the response HTTP status and size.
func (ce *logEntry) appendCombinedToBuf(buf []byte) []byte {
	buf = append(buf, ` "`...)
	buf = AppendQuoted(buf, ce.referer)
	buf = append(buf, `" "`...)
	buf = AppendQuoted(buf, ce.ua)
	buf = append(buf, '"')
	return buf
}

// writeLog writes a log entry for req to w in Apache Common Log Format.
// ts is the timestamp with which the entry should be logged.
// status and size are used to provide the response HTTP status and size.
func (h LogHandler) writeLog(w io.Writer, req *http.Request, ts time.Time, status, size int) {
	le := logEntry{
		username: "-",
		uri:      req.URL.RequestURI(),
		host:     h.remoteHost(req),
		req:      req,
		ts:       ts,
		status:   status,
		size:     size,
	}

	if req.URL.User != nil {
		if name := req.URL.User.Username(); name != "" {
			le.username = name
		}
	}

	bufSize := le.estimateCommonSize()

	if h.isCombined {
		le.referer = req.Referer()
		le.ua = req.UserAgent()
		bufSize += le.estimateCombinedSize()
	}

	if h.SuffixFormatter != nil {
		if se, ok := h.SuffixFormatter.(SizeEstimate); ok {
			bufSize += se.EstimateAppendSize(req, ts, status, size) + 1
		} else {
			bufSize += DefaultAppendSize
		}
	}

	buf := make([]byte, 0, bufSize)

	buf = le.appendCommonToBuf(buf)

	if h.isCombined {
		buf = le.appendCombinedToBuf(buf)
	}

	if h.SuffixFormatter != nil {
		buf = append(buf, ' ')
		buf = h.SuffixFormatter.AppendToLog(buf, req, ts, status, size)
	}

	buf = append(buf, '\n')
	w.Write(buf)
}

// CombinedLoggingHandler return a http.Handler that wraps h and logs requests to out in
// Apache Combined Log Format.
//
// See http://httpd.apache.org/docs/2.2/logs.html#combined for a description of this format.
//
// LoggingHandler always sets the ident field of the log to -
func CombinedLoggingHandler(out io.Writer, h http.Handler) LogHandler {
	return LogHandler{
		writer:     out,
		handler:    h,
		isCombined: true,
	}
}

// LoggingHandler return a http.Handler that wraps h and logs requests to out in
// Apache Common Log Format (CLF).
//
// See http://httpd.apache.org/docs/2.2/logs.html#common for a description of this format.
//
// LoggingHandler always sets the ident field of the log to -
func LoggingHandler(out io.Writer, h http.Handler) LogHandler {
	return LogHandler{
		writer:     out,
		handler:    h,
		isCombined: false,
	}
}
