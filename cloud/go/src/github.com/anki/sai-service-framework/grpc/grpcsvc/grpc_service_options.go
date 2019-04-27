package grpcsvc

import (
	"github.com/anki/sai-service-framework/svc"
	"time"
)

// WithPort specifies the TCP port to listen on. This is provided as
// an alternative to configuration via command line or environment
// variables, primarily for unit testing.
func WithPort(port int) Option {
	return func(c *GrpcServiceComponent) {
		c.port = &port
	}
}

// WithAddr specifies the IP address to listen on. This is
// provided as an alternative to configuration via command line or
// environment variables, primarily for unit testing.
func WithAddr(addr string) Option {
	return func(c *GrpcServiceComponent) {
		c.addr = &addr
	}
}

// WithTLSEnabled specifies whether or not Transport Layer Security
// will be enabled on the listening port. This is provided as an
// alternative to configuration via command line or environment
// variables, primarily for unit testing.
func WithTLSEnabled(value bool) Option {
	return func(c *GrpcServiceComponent) {
		c.tlsConfig.Enabled = &value
	}
}

// WithMaxConcurrentStreams specifies the maximum number of open
// streaming RPC calls allowed.  This is provided as an alternative to
// configuration via command line or environment variables, primarily
// for unit testing.
func WithMaxConcurrentStreams(count int) Option {
	return func(c *GrpcServiceComponent) {
		c.maxConcurrentStreams = &count
	}
}

// WithMaxConnectionAge specifies the maximum time a connection is
// allowed to remain open, regardless of activity. This is provided as
// an alternative to configuration via command line or environment
// variables, primarily for unit testing.
func WithMaxConnectionAge(duration time.Duration) Option {
	d := svc.Duration(duration)
	return func(c *GrpcServiceComponent) {
		c.maxConnectionAge = &d
	}
}

// WithMaxConnectionAge specifies the time a connection will be given
// to shut down before being force closed after it has reached the max
// connection age.  This is provided as an alternative to
// configuration via command line or environment variables, primarily
// for unit testing.
func WithMaxConnectionGracePeriod(duration time.Duration) Option {
	d := svc.Duration(duration)
	return func(c *GrpcServiceComponent) {
		c.maxConnectionGracePeriod = &d
	}
}

// WithMaxConnectionIdle specifies the amount of time a connection is
// allowed to be idle before being shut down.  This is provided as an
// alternative to configuration via command line or environment
// variables, primarily for unit testing.
func WithMaxConnectionIdle(duration time.Duration) Option {
	d := svc.Duration(duration)
	return func(c *GrpcServiceComponent) {
		c.maxConnectionIdle = &d
	}
}

// WithConnectionTimeout specifies the maximum amount of time to allow
// for TLS connection establishment.  This is provided as an
// alternative to configuration via command line or environment
// variables, primarily for unit testing.
func WithConnectionTimeout(duration time.Duration) Option {
	d := svc.Duration(duration)
	return func(c *GrpcServiceComponent) {
		c.connectionTimeout = &d
	}
}

// WithGrpcLog configures whether the gRPC log is redirected to the
// Anki log or not.
func WithGrpcLog(value bool) Option {
	return func(c *GrpcServiceComponent) {
		c.logGrpc = &value
	}
}
