package cloudproc

import (
	pb "github.com/anki/sai-chipper-voice/grpc/pb2"
)

type Handler = pb.IntentService

const (
	HandlerDefault   Handler = pb.IntentService_DEFAULT
	HandlerGoogle    Handler = pb.IntentService_DIALOGFLOW
	HandlerMicrosoft Handler = pb.IntentService_BING_LUIS
)

// Options defines various settings that can be passed in to the cloud process
type Options struct {
	compress bool
	chunkMs  uint
	handler  Handler
}

// SetCompression sets whether compression will be performed on audio
// before uploading (and returns the same Options struct to allow method
// chaining)
func (o *Options) SetCompression(value bool) *Options {
	o.compress = value
	return o
}

// SetChunkMs determines how often the cloud process will stream data to the cloud
func (o *Options) SetChunkMs(value uint) *Options {
	o.chunkMs = value
	return o
}

// SetHandler sets the intent service (MS, Google, etc) that should handle this
// request on the server, if one is desired
func (o *Options) SetHandler(value Handler) *Options {
	o.handler = value
	return o
}
