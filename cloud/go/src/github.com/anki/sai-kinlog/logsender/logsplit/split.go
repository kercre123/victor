/*
Package logsplit provides a LogSplitter type which splits a contiguous text
input into a stream of discrete events, optionally adding a text timestamp
as a prefix for each event.
*/
package logsplit

import (
	"bufio"
	"bytes"
	"fmt"
	"io"
	"regexp"
	"time"
)

var (
	// DefaultSplitter is the default splitter used by LogSplitter.
	DefaultSplitter = new(LineSplitter)
)

// LogSplitter reads data from an io.Reader, splits it into discrete
// events, and writes those events to an io.Writer, with one event per
// Write call.
//
// If Timestamp is provided, it will prefix each generated event with
// a timestamp formatted using the supplied layout.
type LogSplitter struct {
	// Timestamp specifies a time.Format compatible timestamp layout used to
	// prefix each generated event.  No timestamp is added if Timestamp is
	// set to the empty string.
	Timestamp string

	// MaxEventLines specifies the maximum number of output lines that may
	// appear in a single event.  If an event accumulates more than
	// MaxEventLines then it is broken at a line boundary into two events.
	// Set to zero to disable splitting.
	MaxEventLines int

	// Splitter provides the mechanism used to split events.  It defaults
	// to DeafultSplitter.
	Splitter EventSplitter

	// TimeSrc is used by tests to provide a predictable time source.
	// By default time.Now is used
	TimeSrc func() time.Time
}

// Copy reads log data from r and writes complete events to w, with one
// event per call to Write.  It will continue to read until it receives
// io.EOF from the reader, or an error occurs.
//
// It does not call close on either reader or the writer.
func (ls *LogSplitter) Copy(r io.Reader, w io.Writer) error {
	eb := &EventBuffer{
		buf: new(bytes.Buffer),
		w:   w,
		ls:  ls,
		now: ls.TimeSrc,
	}

	if eb.now == nil {
		eb.now = time.Now
	}

	splitter := ls.Splitter
	if splitter == nil {
		splitter = DefaultSplitter
	}

	scanner := bufio.NewScanner(r)
	for scanner.Scan() {
		line := scanner.Bytes()
		if err := splitter.AddLine(eb, line); err != nil {
			return err
		}
	}
	if err := scanner.Err(); err != nil {
		return err
	}
	return eb.Flush()
}

// EventBuffer is used by EventSplitters to accumulate log data for a single
// event.
type EventBuffer struct {
	buf *bytes.Buffer
	ls  *LogSplitter
	now func() time.Time
	w   io.Writer
	lc  int
}

func newEventBuffer(ls *LogSplitter) *EventBuffer {
	return &EventBuffer{
		buf: new(bytes.Buffer),
		ls:  ls,
	}
}

// WriteLine adds a single line of log data to the buffer.  It will
// automatically add a newline to the end of the line.
//
// If the LogSplitter has MaxEventLines set, it will automatically flush the
// event if the limit is reached.
func (eb *EventBuffer) WriteLine(b []byte) error {
	if eb.ls.Timestamp != "" && eb.buf.Len() == 0 {
		fmt.Fprintf(eb.buf, "%s ", eb.now().Format(eb.ls.Timestamp))
	}
	eb.buf.Write(b)
	eb.buf.Write([]byte("\n"))
	eb.lc++
	if eb.ls.MaxEventLines > 0 && eb.lc >= eb.ls.MaxEventLines {
		if err := eb.Flush(); err != nil {
			return err
		}
	}
	return nil
}

// Flush sends the contents of the buffer (if any) as a single event to the
// writer before clearing the buffer ready to receive the next event.
func (eb *EventBuffer) Flush() error {
	if eb.buf.Len() == 0 {
		return nil
	}
	if _, err := eb.w.Write(eb.buf.Bytes()); err != nil {
		return err
	}
	eb.buf.Reset()
	eb.lc = 0
	return nil
}

// EventSplitter is implemented by types responsible for splitting a log stream
// into discrete events.
type EventSplitter interface {
	// AddLine is called by the LogSplitter when a new line has been read
	// from the input.  The EventSplitter may add the line to the existing
	// EventBuffer provided, and/or flush the EventBuffer to emit an event.
	AddLine(buf *EventBuffer, line []byte) error
}

// LineSplitter generates a discrete event after every newline.
// Empty lines are discarded.
type LineSplitter struct{}

// AddLine implements the EventSplitter interface
func (s *LineSplitter) AddLine(eb *EventBuffer, line []byte) error {
	if len(line) == 0 {
		return nil
	}
	if err := eb.WriteLine(line); err != nil {
		return err
	}
	return eb.Flush()
}

// RegexSplitter generates a discrete event whenever a line matches the
// supplied regular expression.  The matching line is then used as the first
// line of the next event.
// Empty lines are not discarded.
type RegexSplitter struct {
	r *regexp.Regexp
}

// NewRegexSplitter creates a new RegexSplitter using the supplied regular
// expression.
func NewRegexSplitter(r *regexp.Regexp) *RegexSplitter {
	return &RegexSplitter{r: r}
}

// AddLine implements the EventSplitter interface
func (rs *RegexSplitter) AddLine(eb *EventBuffer, line []byte) error {
	if rs.r.Match(line) {
		if err := eb.Flush(); err != nil {
			return err
		}
	}
	return eb.WriteLine(line)
}

// NoSplit consumes the entire input and emits it as a single (possibly
// multi-line) event.  The event will still be split should it reach
// the MaxEventLines threshold.
type NoSplit struct{}

// AddLine implements the EventSplitter interface
func (s *NoSplit) AddLine(eb *EventBuffer, line []byte) error {
	return eb.WriteLine(line)
}
