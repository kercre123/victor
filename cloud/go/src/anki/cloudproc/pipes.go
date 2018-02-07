package cloudproc

import (
	"anki/ipc"
	"anki/util"
	"strings"
)

type MicPipe interface {
	Hotword() chan struct{}
	Audio() chan socketMsg
	WriteMic(buf []byte) (int, error)
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

type ipcPipe struct {
	hotword chan struct{}
	audio   chan socketMsg
	conn    ipc.Conn
}

func (p *ipcPipe) Hotword() chan struct{} {
	return p.hotword
}

func (p *ipcPipe) Audio() chan socketMsg {
	return p.audio
}

func (p *ipcPipe) WriteMic(buf []byte) (int, error) {
	return p.conn.Write(buf)
}

func NewIpcPipe(mic ipc.Conn, testChan chan []byte, kill <-chan struct{}) MicPipe {
	pipe := &ipcPipe{hotword: make(chan struct{}),
		audio: make(chan socketMsg),
		conn:  mic}
	go socketReader(mic, pipe.hotword, pipe.audio, kill)

	// if test channel is provided, multiplex it with the mic channel
	if testChan != nil {
		go func() {
			for {
				select {
				case buf := <-testChan:
					directSocketMessage(buf, pipe.hotword, pipe.audio)
				case <-kill:
					return
				}
			}
		}()
	}

	return pipe
}
