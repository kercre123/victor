package harness

import (
	"clad/cloud"
	"io"

	"anki/cloudproc"
)

type Harness interface {
	cloudproc.MsgIO
	io.Closer
	ReadMessage() (*cloud.Message, error)
}

type memHarness struct {
	cloudproc.MsgIO
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

	io, receiver := cloudproc.NewMemPipe()
	process := &cloudproc.Process{}
	process.AddReceiver(receiver)
	process.AddIntentWriter(&cloudproc.ChanMsgSender{Ch: intentResult})

	go process.Run(append(options, cloudproc.WithStopChannel(kill))...)

	return &memHarness{
		MsgIO:  io,
		kill:   kill,
		intent: intentResult}, nil
}
