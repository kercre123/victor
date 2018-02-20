package cloudproc

import (
	"anki/ipc"
	"anki/util"
)

// Sender defines the API for sending audio data from an external source into
// the cloud process
type Sender interface {
	SendHotword() error
	SendAudio(buf []byte) (int, error)
	Read() ([]byte, error)
}

type ipcSender struct {
	conn ipc.Conn
}

func (s *ipcSender) SendHotword() error {
	_, err := s.conn.Write([]byte(HotwordMessage))
	return err
}

func (s *ipcSender) SendAudio(buf []byte) (int, error) {
	return s.conn.Write(buf)
}

func (s *ipcSender) Read() ([]byte, error) {
	return s.conn.ReadBlock(), nil
}

// NewIpcSender returns a sender that uses the given IPC connection to
// send audio data to the cloud process
func NewIpcSender(conn ipc.Conn) Sender {
	return &ipcSender{conn}
}

type memSender struct {
	recv chan []byte
	dest *Receiver
}

func (s *memSender) SendHotword() error {
	s.dest.hotword <- struct{}{}
	return nil
}

func (s *memSender) SendAudio(buf []byte) (int, error) {
	s.dest.audio <- socketMsg{buf}
	return len(buf), nil
}

func (s *memSender) Read() ([]byte, error) {
	buf := <-s.recv
	return buf, nil
}

// NewMemPipe returns a connected pair of a Sender and a Receiver that directly
// transmit data over channels; the Receiver should be passed in to the cloud
// process to get data from the Sender
func NewMemPipe() (Sender, *Receiver) {
	sender := &memSender{make(chan []byte), nil}
	receiver := &Receiver{hotword: make(chan struct{}),
		audio:  make(chan socketMsg),
		writer: util.AsyncWriter(util.NewChanWriter(sender.recv))}
	sender.dest = receiver
	return sender, receiver
}
