package main

import (
	"anki/ipc"
	"fmt"
)

func main() {
	fmt.Println("Hello, world!")
	micSock, err := ipc.NewClientSocket("0.0.0.0", 12345)
	if err != nil {
		fmt.Println("Couldn't create mic socket:", err)
		return
	}

	aiSock, err := ipc.NewClientSocket("0.0.0.0", 12346)
	if err != nil {
		fmt.Println("Couldn't create AI socket:", err)
		return
	}

	runProcess(micSock, aiSock)
}
