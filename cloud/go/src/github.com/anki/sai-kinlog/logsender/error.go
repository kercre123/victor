// Copyright 2016 Anki, Inc.

package logsender

import "fmt"

// FlushFailureAction specifies what should happen if a hard error occurs
// while attempting to flush data to Kinesis.
type FlushFailureAction int

const (
	// FlushRetry will cause a failed flush to be retried continuously until
	// it succeeds.
	FlushRetry FlushFailureAction = iota + 1

	// FlushDiscard will caused failed flush records to be discarded.
	FlushDiscard
)

// An ErrorHandler provides a function for gracefully handling hard flush errors
// where all Kinesis repeat attempts have failed.
//
// The handler is called directly by the flushing goroutine.  It may fix the
// Kinesis connection and return FlushRetry, decide to drop the data by
// returning FlushDiscard or abort the program by panicing.
type ErrorHandler interface {
	FlushFailed(s *Sender, err error) FlushFailureAction
}

// ErrorHandlerFunc adapts a regular function to comply with the ErrorHandler
// interface.
type ErrorHandlerFunc func(s *Sender, err error) FlushFailureAction

// FlushFailed calls f(s, err)
func (f ErrorHandlerFunc) FlushFailed(s *Sender, err error) FlushFailureAction {
	return f(s, err)
}

type defaultErrorHandler struct{}

func (h *defaultErrorHandler) FlushFailed(s *Sender, err error) FlushFailureAction {
	panic(fmt.Sprintf("kinlog flush to kinesis failed: %s", err))
}
