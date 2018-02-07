package harness

import (
	"fmt"

	"anki/cloudproc"
	"anki/ipc"
	"anki/util"
)

type Harness struct {
	Mic   ipc.Conn
	AI    ipc.Conn
	serv1 ipc.Server
	serv2 ipc.Server
	kill  chan struct{}
}

var aiSockName = ipc.GetSocketPath("ai_harness_sock")
var micSockName = ipc.GetSocketPath("mic_harness_sock")

func CreateProcess() (*Harness, error) {
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
	micPipe := cloudproc.NewIpcPipe(micClient, nil, kill)

	go cloudproc.RunProcess(micPipe, aiClient, kill)

	return &Harness{
		Mic:   micConn,
		AI:    aiConn,
		serv1: aiServer,
		serv2: micServer,
		kill:  kill}, nil
}

func (h *Harness) Close() {
	close(h.kill)
	h.Mic.Close()
	h.AI.Close()
	h.serv1.Close()
	h.serv2.Close()
}
