package main

import (
	"anki/cloudproc"
	"anki/ipc"
	"fmt"
	"time"
)

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

func main() {
	fmt.Println("Hello, world!")
	micSock := getSocketWithRetry("/dev/mic_sock", "cp_mic")
	aiSock := getSocketWithRetry("/dev/ai_sock", "cp_ai")
	fmt.Println("Sockets successfully created")

	cloudproc.RunProcess(micSock, aiSock, nil)
}
