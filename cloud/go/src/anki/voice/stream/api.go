package stream

import (
	"anki/util"
	"bytes"
	"context"
	"encoding/binary"
)

func NewStreamer(ctx context.Context, receiver Receiver, streamSize int, opts ...Option) *Streamer {
	strm := &Streamer{
		conn:        nil,
		stream:      nil,
		byteChan:    make(chan []byte),
		audioStream: make(chan []byte, 10),
		receiver:    receiver}

	for _, o := range opts {
		o(&strm.opts)
	}

	strm.ctx, strm.cancel = context.WithCancel(ctx)

	go strm.init(streamSize)
	return strm
}

func (strm *Streamer) AddSamples(samples []int16) {
	var buf bytes.Buffer
	binary.Write(&buf, binary.LittleEndian, samples)
	strm.AddBytes(buf.Bytes())
}

func (strm *Streamer) AddBytes(bytes []byte) {
	strm.byteChan <- bytes
}

func (strm *Streamer) Close() error {
	strm.closed = true
	strm.cancel()
	var err util.Errors
	if strm.stream != nil {
		err.Append(strm.stream.Close())
	}
	if strm.conn != nil {
		err.Append(strm.conn.Close())
	}
	return err.Error()
}

// SetVerbose enables or disables verbose logging
func SetVerbose(value bool) {
	verbose = value
}
