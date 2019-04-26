package testclient

import pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"

type ClientOpt func(o *clientOpts)

func WithDeviceId(id string) ClientOpt {
	return func(o *clientOpts) {
		o.DeviceId = id
	}
}

func WithSession(id string) ClientOpt {
	return func(o *clientOpts) {
		o.Session = id
	}
}

func WithMode(m pb.RobotMode) ClientOpt {
	return func(o *clientOpts) {
		o.Mode = m
	}
}

func WithAudioEncoding(a pb.AudioEncoding) ClientOpt {
	return func(o *clientOpts) {
		o.Audio = a
	}
}

func WithLangCode(l pb.LanguageCode) ClientOpt {
	return func(o *clientOpts) {
		o.LangCode = l
	}
}

func WithService(s pb.IntentService) ClientOpt {
	return func(o *clientOpts) {
		o.Service = s
	}
}

func WithSingleUtterance(s bool) ClientOpt {
	return func(o *clientOpts) {
		o.SingleUtterance = s
	}
}

func WithSkipDas(s bool) ClientOpt {
	return func(o *clientOpts) {
		o.SkipDas = s
	}
}

func WithInsecure(s bool) ClientOpt {
	return func(o *clientOpts) {
		o.Insecure = s
	}
}

func WithAuthConfig(c AuthConfig) ClientOpt {
	return func(o *clientOpts) {
		o.AuthCfg = c
	}
}

func WithFirmware(fw string) ClientOpt {
	return func(o *clientOpts) {
		o.Fw = fw
	}
}

func WithSaveAudio(s bool) ClientOpt {
	return func(o *clientOpts) {
		o.SaveAudio = s
	}
}
