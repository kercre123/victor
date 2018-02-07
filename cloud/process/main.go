package main

import (
	"anki/cloudproc"
	"anki/ipc"
	"flag"
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

func testReader(serv ipc.Server, ch chan []byte) {
	for conn := range serv.NewConns() {
		go func(conn ipc.Conn) {
			for {
				msg := conn.ReadBlock()
				if msg == nil || len(msg) == 0 {
					conn.Close()
					return
				}
				ch <- msg
			}
		}(conn)
	}
}

func main() {
	fmt.Println("Hello, world!")
	micSock := getSocketWithRetry(ipc.GetSocketPath("mic_sock"), "cp_mic")
	defer micSock.Close()
	aiSock := getSocketWithRetry(ipc.GetSocketPath("ai_sock"), "cp_ai")
	defer aiSock.Close()
	testSock, err := ipc.NewUnixgramServer(ipc.GetSocketPath("cp_test"))
	if err != nil {
		fmt.Println("Server create error:", err)
	}
	defer testSock.Close()
	fmt.Println("Sockets successfully created")

	testChan := make(chan []byte)
	defer close(testChan)
	go testReader(testSock, testChan)

	flag.BoolVar(&verbose, "verbose", false, "enable verbose logging")
	flag.Parse()
	cloudproc.SetVerbose(verbose)
	micPipe := cloudproc.NewIpcPipe(micSock, testChan, nil)
	cloudproc.RunProcess(micPipe, aiSock, nil)
}
