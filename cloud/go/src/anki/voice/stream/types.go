package stream

import (
	"clad/cloud"
	"context"
	"sync"

	"github.com/anki/sai-chipper-voice/client/chipper"
)

type Streamer struct {
	conn        *chipper.Conn
	stream      chipper.Stream
	byteChan    chan []byte
	audioStream chan []byte
	respOnce    sync.Once
	closed      bool
	opts        options
	receiver    Receiver
	ctx         context.Context
	cancel      func()
}

type Receiver interface {
	OnError(cloud.ErrorType, error)
	OnStreamOpen(string)
	OnIntent(*cloud.IntentResult)
	OnConnectionResult(*cloud.ConnectionResult)
}
