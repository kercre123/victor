package cloudproc

import (
	"anki/voice"
)

type Option func(o *options)

type options struct {
	voice     *voice.Process
	voiceOpts []voice.Option
	stop      <-chan struct{}
}

func WithStopChannel(stop <-chan struct{}) Option {
	return func(o *options) {
		o.stop = stop
	}
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
