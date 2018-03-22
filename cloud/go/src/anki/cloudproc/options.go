package cloudproc

import (
	pb "github.com/anki/sai-chipper-voice/grpc/pb"
)

type Handler = pb.IntentService

const (
	HandlerDefault   Handler = pb.IntentService_DEFAULT
	HandlerGoogle    Handler = pb.IntentService_DIALOGFLOW
	HandlerMicrosoft Handler = pb.IntentService_BING_LUIS
)

type Option func(o *options)

type options struct {
	compress bool
	chunkMs  uint
	handler  Handler
	stop     <-chan struct{}
}

// WithCompression sets whether compression will be performed on audio
// before uploading (and returns the same Options struct to allow method
// chaining)
func WithCompression(value bool) Option {
	return func(o *options) {
		o.compress = value
	}
}

// WithChunkMs determines how often the cloud process will stream data to the cloud
func WithChunkMs(value uint) Option {
	return func(o *options) {
		o.chunkMs = value
	}
}

// WithHandler sets the intent service (MS, Google, etc) that should handle this
// request on the server, if one is desired
func WithHandler(value Handler) Option {
	return func(o *options) {
		o.handler = value
	}
}

// WithStopChannel sets a channel that can be triggered to kill the process
func WithStopChannel(value <-chan struct{}) Option {
	return func(o *options) {
		o.stop = value
	}
}
