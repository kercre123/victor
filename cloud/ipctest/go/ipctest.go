package main

import (
	"anki/ipc"
	"bufio"
	"fmt"
	"os"
	"strings"
)

const _Port = 19412

func messageSender(sock ipc.Conn) {
	for {
		reader := bufio.NewReader(os.Stdin)
		text, _ := reader.ReadString('\n')
		text = strings.TrimRight(text, "\n")
		if text == "done" {
			os.Exit(0)
		}
		_, err := sock.Write([]byte(text))
		if err != nil {
			fmt.Println("Error sending:", err)
		}
	}
}

func runServer() {
	fmt.Println("Starting server mode, waiting for connection")
	serv, _ := ipc.NewUDPServer(_Port)
	conn := <-serv.NewConns()
	fmt.Println("Connected!")
	go messageSender(conn)
	for {
		msg := conn.ReadBlock()
		fmt.Println("Message received:", string(msg))
	}
}

func runClient() {
	fmt.Println("Starting client mode (should be done after server is started)")
	sock, _ := ipc.NewUDPClient("127.0.0.1", _Port)
	go messageSender(sock)
	for {
		msg := sock.ReadBlock()
		fmt.Println("Message received:", string(msg))
	}
}

func main() {
	fmt.Println("IPC test: Go edition")
	args := os.Args[1:]
	if len(args) == 0 || args[0] == "server" {
		runServer()
	} else {
		runClient()
	}
}
