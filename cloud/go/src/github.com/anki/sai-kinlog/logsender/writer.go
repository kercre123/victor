// Copyright 2016 Anki, Inc.

package logsender

import (
	"errors"
	"sync/atomic"
	"time"

	"github.com/anki/sai-kinlog/logsender/logrecord"
	"github.com/aws/aws-sdk-go/aws"
)

// ErrWriterClosed is returned by EventWriter.Write
// if a write is attempted after Close has  been called.
var ErrWriterClosed = errors.New("writer is closed")

// EventWriter receives pre-broken events and streams them to Kinesis.
type EventWriter struct {
	md           logrecord.Metadata
	addTimestamp bool
	s            *Sender
	closed       int32
}

// Write sends log events to Kinesis - Each call Write must hold a single,
// complete (ie. "broken") event.  Events may consists of multiple lines.
//
// If each call to Write is not guaranteed to contain exactly one event, use
// StreamWriter instead.
//
// If the outbound buffer is full then Write will return an ErrQueueFull error;
// it will not block.
func (w *EventWriter) Write(p []byte) (n int, err error) {
	if atomic.LoadInt32(&w.closed) > 0 {
		return 0, ErrWriterClosed
	}

	e := logrecord.LogEntry{
		Metadata: w.md,
		Data: logrecord.Data{
			IsDone: true,
			Data:   append([]byte{}, p...),
		},
	}
	if w.addTimestamp {
		e.Data.Time = aws.Time(time.Now())
	}
	err = w.s.write(e)
	if err != nil {
		return 0, err
	}
	return len(p), nil
}

// Close causes further writes to return ErrWriterClosed.
// It is safe to call from any goroutine.
func (w *EventWriter) Close() error {
	atomic.StoreInt32(&w.closed, 1)
	return nil
}
