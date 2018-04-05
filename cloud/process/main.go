package main

import (
	"anki/cloudproc"
	"anki/ipc"
	"fmt"
	"time"
)

var verbose bool

func getSocketWithRetry(name string, client string) ipc.Conn {
	for {
		sock, err := ipc.NewUnixgramClient(name, client)
		if err != nil {
			fmt.Println("Couldn't create socket", name, "- retrying:", err)
			time.Sleep(5 * time.Second)
		} else {
			return sock
		}
	}
}

func testReader(serv ipc.Server, send cloudproc.Sender) {
	for conn := range serv.NewConns() {
		go func(conn ipc.Conn) {
			for {
				msg := conn.ReadBlock()
				if msg == nil || len(msg) == 0 {
					conn.Close()
					return
				}
				if string(msg) == cloudproc.HotwordMessage {
					send.SendHotword()
				} else {
					send.SendAudio(msg)
				}
			}
		}(conn)
	}
}

func main() {
	fmt.Println("Hello, world!")

	// don't yet have control over process startup on DVT2, set these as default
	verbose = true
	test := true

	// flag.BoolVar(&verbose, "verbose", false, "enable verbose logging")
	// var test bool
	// flag.BoolVar(&test, "test", false, "enable test channel")
	// flag.Parse()

	micSock := getSocketWithRetry(ipc.GetSocketPath("mic_sock"), "cp_mic")
	defer micSock.Close()
	aiSock := getSocketWithRetry(ipc.GetSocketPath("ai_sock"), "cp_ai")
	defer aiSock.Close()

	// set up test channel if flags say we should
	var testRecv *cloudproc.Receiver
	if test {
		testSock, err := ipc.NewUnixgramServer(ipc.GetSocketPath("cp_test"))
		if err != nil {
			fmt.Println("Server create error:", err)
		}
		defer testSock.Close()

		var testSend cloudproc.Sender
		testSend, testRecv = cloudproc.NewMemPipe()
		go testReader(testSock, testSend)
		fmt.Println("Test channel created")
	}
	fmt.Println("Sockets successfully created")

	cloudproc.SetVerbose(verbose)
	receiver := cloudproc.NewIpcReceiver(micSock, nil)

	process := &cloudproc.Process{}
	process.AddReceiver(receiver)
	if testRecv != nil {
		process.AddReceiver(testRecv)
	}
	process.AddIntentWriter(aiSock)
	process.Run(nil)
}
