package harness

import (
	"clad/cloud"
	"io"

	"anki/cloudproc"
	"anki/voice"
)

type Harness interface {
	voice.MsgIO
	io.Closer
	ReadMessage() (*cloud.Message, error)
}

type memHarness struct {
	voice.MsgIO
	kill   chan struct{}
	intent chan *cloud.Message
}

func (h *memHarness) Close() error {
	close(h.kill)
	return nil
}

func (h *memHarness) ReadMessage() (*cloud.Message, error) {
	return <-h.intent, nil
}

func CreateMemProcess(options ...cloudproc.Option) (Harness, error) {
	kill := make(chan struct{})

	intentResult := make(chan *cloud.Message)

	io, receiver := voice.NewMemPipe()
	process := &voice.Process{}
	process.AddReceiver(receiver)
	process.AddIntentWriter(&voice.ChanMsgSender{Ch: intentResult})

	options = append(options, cloudproc.WithStopChannel(kill))
	options = append(options, cloudproc.WithVoice(process))

	go cloudproc.Run(options...)

	return &memHarness{
		MsgIO:  io,
		kill:   kill,
		intent: intentResult}, nil
}
