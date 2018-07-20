package stream

import (
	"anki/util"
	"bytes"
	"encoding/binary"
)

func NewStreamer(receiver Receiver, streamSize int, opts ...Option) *Streamer {
	strm := &Streamer{
		conn:        nil,
		stream:      nil,
		byteChan:    make(chan []byte),
		audioStream: make(chan []byte, 10),
		done:        make(chan struct{}),
		receiver:    receiver}

	for _, o := range opts {
		o(&strm.opts)
	}

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
	close(strm.done)
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
