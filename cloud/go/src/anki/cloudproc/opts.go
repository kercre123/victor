package cloudproc

import (
	"anki/jdocs"
	"anki/token"
	"anki/voice"
)

type Option func(o *options)

type options struct {
	voice     *voice.Process
	voiceOpts []voice.Option
	tokenOpts []token.Option
	jdocOpts  []jdocs.Option
}

func WithVoice(process *voice.Process) Option {
	return func(o *options) {
		o.voice = process
	}
}

func WithVoiceOptions(voiceOptions ...voice.Option) Option {
	return func(o *options) {
		o.voiceOpts = append(o.voiceOpts, voiceOptions...)
	}
}

func WithTokenOptions(tokenOptions ...token.Option) Option {
	return func(o *options) {
		o.tokenOpts = append(o.tokenOpts, tokenOptions...)
	}
}

func WithJdocs(jdocOptions ...jdocs.Option) Option {
	return func(o *options) {
		o.jdocOpts = append(o.jdocOpts, jdocOptions...)
		if o.jdocOpts == nil {
			// even if no options specified, code is saying "run jdocs plz" by calling this
			o.jdocOpts = []jdocs.Option{}
		}
	}
}
