package harness

import (
	"fmt"
	"io"

	"anki/cloudproc"
	"anki/ipc"
	"anki/util"
)

type Harness interface {
	cloudproc.Sender
	io.Closer
	ReadIntent() (string, error)
}

type ipcHarness struct {
	cloudproc.Sender
	mic   ipc.Conn
	ai    ipc.Conn
	serv1 ipc.Server
	serv2 ipc.Server
	kill  chan struct{}
}

var aiSockName = ipc.GetSocketPath("ai_harness_sock")
var micSockName = ipc.GetSocketPath("mic_harness_sock")

func CreateIpcProcess() (Harness, error) {
	aiServer, err1 := ipc.NewUnixServer(aiSockName)
	micServer, err2 := ipc.NewUnixServer(micSockName)

	aiClient, err3 := ipc.NewUnixClient(aiSockName)
	micClient, err4 := ipc.NewUnixClient(micSockName)

	errs := util.Errors{}
	errs.AppendMulti(err1, err2, err3, err4)
	if err := errs.Error(); err != nil {
		fmt.Println("Socket error:", err)
		return nil, err
	}

	fmt.Println("Getting harness connections...")

	micConn := <-micServer.NewConns()
	aiConn := <-aiServer.NewConns()

	fmt.Println("Got harness connections")

	kill := make(chan struct{})
	receiver := cloudproc.NewIpcReceiver(micClient, kill)

	process := &cloudproc.Process{}
	process.AddReceiver(receiver)
	process.AddIntentWriter(aiClient)
	go process.Run(kill)

	sender := cloudproc.NewIpcSender(micConn)

	return &ipcHarness{
		Sender: sender,
		mic:    micConn,
		ai:     aiConn,
		serv1:  aiServer,
		serv2:  micServer,
		kill:   kill}, nil
}

func (h *ipcHarness) ReadIntent() (string, error) {
	return string(h.ai.ReadBlock()), nil
}

func (h *ipcHarness) Close() error {
	close(h.kill)
	h.mic.Close()
	h.ai.Close()
	h.serv1.Close()
	h.serv2.Close()
	return nil
}

type memHarness struct {
	cloudproc.Sender
	kill   chan struct{}
	intent chan []byte
}

func (h *memHarness) Close() error {
	close(h.kill)
	return nil
}

func (h *memHarness) ReadIntent() (string, error) {
	return string(<-h.intent), nil
}

func CreateMemProcess() (Harness, error) {
	kill := make(chan struct{})

	intentResult := make(chan []byte)

	sender, receiver := cloudproc.NewMemPipe()
	process := &cloudproc.Process{}
	process.AddReceiver(receiver)
	process.AddIntentWriter(util.NewChanWriter(intentResult))
	go process.Run(kill)

	return &memHarness{
		Sender: sender,
		kill:   kill,
		intent: intentResult}, nil
}
