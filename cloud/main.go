package main

import (
	"bufio"
	"flag"
	"fmt"
	"os"
)

func messageSender(ch chan<- []byte) {
	for {
		reader := bufio.NewReader(os.Stdin)
		text, _ := reader.ReadString('\n')
		fmt.Println("Sending:", text)
		ch <- []byte(text)
	}
}

func server() {
	fmt.Println("Starting server")
	sock := NewServerSocket(12345)
	go messageSender(sock.write)
	for {
		fmt.Println("Got message:", string(<-sock.read))
	}
}

func client() {
	fmt.Println("Starting client")
	sock := NewClientSocket("127.0.0.1", 12345)
	sock.write <- []byte("Hello, server!")
	go messageSender(sock.write)
	for {
		fmt.Println("Got message:", string(<-sock.read))
	}
}

func main() {
	mode := flag.String("mode", "server", "server or client (default server)")
	flag.Parse()

	switch *mode {
	case "server":
		server()
	default:
		client()
	}
}
