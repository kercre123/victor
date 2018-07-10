package token

type options struct {
	stop <-chan struct{}
}

// Option defines an option that can be set on the token server
type Option func(o *options)

// WithStopChannel sets a channel that can be triggered to kill the process
func WithStopChannel(value <-chan struct{}) Option {
	return func(o *options) {
		o.stop = value
	}
}
