package cloudproc

import (
	"anki/ipc"
	"anki/util"
	"io"
	"strings"
)

// Receiver is an object that should be passed in to the cloud process
// and determines how it will receive audio data from external sources
type Receiver struct {
	hotword chan struct{}
	audio   chan socketMsg
	writer  io.Writer
}

func (r *Receiver) writeBack(buf []byte) (int, error) {
	return r.writer.Write(buf)
}

// bufToGoString converts byte buffers that may be null-terminated if created in C
// to Go strings by trimming off null chars
func bufToGoString(buf []byte) string {
	return strings.Trim(string(buf), "\x00")
}

func directSocketMessage(buf []byte, hotword chan struct{}, audio chan socketMsg) {
	if buf != nil && len(buf) > 0 {
		if bufToGoString(buf) == HotwordMessage {
			hotword <- struct{}{}
		} else {
			audio <- socketMsg{buf}
		}
	}
}

// reads messages from a socket and places them on the channel
func socketReader(s ipc.Conn, hotword chan struct{}, audio chan socketMsg, kill <-chan struct{}) {
	for {
		if util.CanSelect(kill) {
			return
		}

		buf := s.ReadBlock()
		directSocketMessage(buf, hotword, audio)
	}
}

// NewIpcReceiver constructs a Receiver that receives audio data and hotword signals
// over the given IPC connection
func NewIpcReceiver(conn ipc.Conn, kill <-chan struct{}) *Receiver {
	hotword := make(chan struct{})
	audio := make(chan socketMsg)
	recv := &Receiver{hotword: hotword,
		audio:  audio,
		writer: conn}
	go socketReader(conn, hotword, audio, kill)

	return recv
}
