package main

import (
	"anki/ipc"
	"bufio"
	"flag"
	"fmt"
	"os"
)

func messageSender(sock ipc.Socket) {
	for {
		reader := bufio.NewReader(os.Stdin)
		text, _ := reader.ReadString('\n')
		fmt.Println("Sending:", text)
		_, err := sock.Write([]byte(text))
		if err != nil {
			fmt.Println("Error sending:", err)
		}
	}
}

func runServer() {
	fmt.Println("Starting server")
	sock, _ := ipc.NewServerSocket(12345)
	go messageSender(sock)
	for {
		_, msg := sock.ReadBlock()
		fmt.Println("Got message:", string(msg))
	}
}

func runClient() {
	fmt.Println("Starting client")
	sock, _ := ipc.NewClientSocket("127.0.0.1", 12345)
	go messageSender(sock)
	for {
		_, msg := sock.ReadBlock()
		fmt.Println("Got message:", string(msg))
	}
}

func main() {
	mode := flag.String("mode", "server", "server or client (default server)")
	flag.Parse()

	switch *mode {
	case "server":
		runServer()
	default:
		runClient()
	}
	fmt.Println("Hello, world!")
}
