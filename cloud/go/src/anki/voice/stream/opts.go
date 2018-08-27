package stream

import (
	"anki/token"
	"clad/cloud"

	"github.com/anki/sai-chipper-voice/client/chipper"
)

var platformOpts []chipper.ConnOpt

type options struct {
	tokener      token.Accessor
	requireToken bool
	mode         cloud.StreamType
	connOpts     []chipper.ConnOpt
	streamOpts   chipper.IntentOpts
	checkOpts    *chipper.ConnectOpts
	url          string
	secret       string
}

type Option func(o *options)

func WithTokener(t token.Accessor, require bool) Option {
	return func(o *options) {
		o.tokener = t
		o.requireToken = require
	}
}

func WithIntentOptions(opts chipper.IntentOpts, mode cloud.StreamType) Option {
	return func(o *options) {
		o.mode = mode
		o.streamOpts = opts
	}
}

func WithKnowledgeGraphOptions(opts chipper.StreamOpts) Option {
	return func(o *options) {
		o.mode = cloud.StreamType_KnowledgeGraph
		o.streamOpts.StreamOpts = opts
	}
}

func WithConnectionCheckOptions(opts chipper.ConnectOpts) Option {
	return func(o *options) {
		o.checkOpts = &opts
	}
}

func WithChipperURL(url string) Option {
	return func(o *options) {
		o.url = url
	}
}

func WithChipperSecret(secret string) Option {
	return func(o *options) {
		o.secret = secret
	}
}
