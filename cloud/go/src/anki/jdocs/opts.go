package jdocs

import "anki/token"

type options struct {
	server  bool
	tokener token.Accessor
}

// Option defines an option that can be set on the token server
type Option func(o *options)

// WithServer specifies that an IPC server should be started so other processes
// can request jdocs from this process
func WithServer() Option {
	return func(o *options) {
		o.server = true
	}
}

// WithTokener specifies that the given token.Accessor should be used to obtain
// authorization credentials
func WithTokener(value token.Accessor) Option {
	return func(o *options) {
		o.tokener = value
	}
}
