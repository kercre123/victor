package stream

import (
	"anki/util"
	"bytes"
	"context"
	"encoding/binary"
	"errors"
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

	if timeout := strm.opts.streamOpts.Timeout; timeout != 0 {
		strm.ctx, strm.cancel = context.WithTimeout(ctx, timeout)
	} else {
		strm.ctx, strm.cancel = context.WithCancel(ctx)
	}

	go strm.init(streamSize)
	return strm
}

func (strm *Streamer) AddSamples(samples []int16) {
	if strm.opts.checkOpts != nil {
		// no external audio input during connection check
		return
	}
	var buf bytes.Buffer
	binary.Write(&buf, binary.LittleEndian, samples)
	strm.addBytes(buf.Bytes())
}

func (strm *Streamer) AddBytes(bytes []byte) {
	if strm.opts.checkOpts != nil {
		// no external audio input during connection check
		return
	}
	strm.addBytes(bytes)
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

func (strm *Streamer) CloseSend() error {
	// ignore if conn check?
	if strm.stream != nil {
		return strm.stream.CloseSend()
	}
	return errors.New("cannot CloseSend on nil stream")
}

// SetVerbose enables or disables verbose logging
func SetVerbose(value bool) {
	verbose = value
}
