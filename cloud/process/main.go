package main

import (
	"anki/cloudproc"
	"anki/ipc"
	"fmt"
)

func main() {
	fmt.Println("Hello, world!")
	micSock, err := ipc.NewUDPClient("0.0.0.0", 9880)
	if err != nil {
		fmt.Println("Couldn't create mic socket:", err)
		return
	}

	aiSock, err := ipc.NewUDPClient("0.0.0.0", 12345)
	if err != nil {
		fmt.Println("Couldn't create AI socket:", err)
		return
	}

	cloudproc.RunProcess(micSock, aiSock, nil)
}
