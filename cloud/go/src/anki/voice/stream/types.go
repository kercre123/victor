package stream

import (
	"clad/cloud"
	"context"
	"sync"
)

type Streamer struct {
	conn        Conn
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

type CloudError struct {
	Kind cloud.ErrorType
	Err  error
}
