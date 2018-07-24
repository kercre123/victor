package token

type options struct {
	stop   <-chan struct{}
	server bool
}

// Option defines an option that can be set on the token server
type Option func(o *options)

// WithStopChannel sets a channel that can be triggered to kill the process
func WithStopChannel(value <-chan struct{}) Option {
	return func(o *options) {
		o.stop = value
	}
}

// WithServer specifies that an IPC server should be started so other processes
// can request tokens from this process
func WithServer() Option {
	return func(o *options) {
		o.server = true
	}
}
