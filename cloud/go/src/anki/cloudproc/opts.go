package cloudproc

import (
	"anki/token"
	"anki/voice"
)

type Option func(o *options)

type options struct {
	voice     *voice.Process
	voiceOpts []voice.Option
	tokenOpts []token.Option
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

func WithToken(tokenOptions ...token.Option) Option {
	return func(o *options) {
		o.tokenOpts = append(o.tokenOpts, tokenOptions...)
	}
}
