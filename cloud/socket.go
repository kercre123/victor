package main

import (
	"errors"
	"fmt"
	"net"
	"time"
)

// Socket provides an interface for inter-process communication via sockets.
// Servers are assumed to be connected to only one client, so writing doesn't have
// any notion of a destination; clients send messages to servers, and servers send
// messages back to the client that connected to them.
type Socket interface {
	Read() (n int, b []byte)           // Reads data from the socket if available, otherwise returns (0, nil)
	ReadBlock() (n int, b []byte)      // Reads data from the socket, blocking until data becomes available
	Write(b []byte) (n int, err error) // Write a message to the socket
}

type socketBase struct {
	read chan []byte
}

type clientSocket struct {
	socketBase
	conn net.Conn
}

func (s socketBase) Read() (n int, b []byte) {
	v, ok := <-s.read
	if !ok {
		return 0, nil
	}
	return len(v), v
}

func (s socketBase) ReadBlock() (n int, b []byte) {
	v := <-s.read
	return len(v), v
}

func (s clientSocket) Write(b []byte) (n int, err error) {
	return s.conn.Write(b)
}

// NewClientSocket returns a new Socket that will attempt to connect to the server
// on the given IP and port
func NewClientSocket(ip string, port int) (Socket, error) {
	conn, err := net.Dial("udp", fmt.Sprintf("%v:%v", ip, port))
	if err != nil {
		return nil, errors.New(fmt.Sprint("Couldn't connect:", err))
	}
	conn.SetDeadline(time.Time{})
	read := make(chan []byte)
	sock := clientSocket{socketBase{read}, conn}

	// Send one byte to server so it marks us as connected
	// (behavior defined by UdpClient.cpp)
	conn.Write(make([]byte, 1))

	// reader thread
	go func() {
		for {
			buf := make([]byte, 2048)
			n, err := conn.Read(buf)
			if err != nil {
				fmt.Println("Couldn't read:", err)
				break
			}
			read <- buf[:n]
		}
	}()

	return sock, nil
}

type serverSocket struct {
	socketBase
	conn  net.PacketConn
	addr  net.Addr
	ready bool
}

func (s serverSocket) Write(b []byte) (n int, err error) {
	if !s.ready {
		return 0, errors.New("Can't write to unconnected server socket")
	}
	return s.conn.WriteTo(b, s.addr)
}

// NewServerSocket returns a new server socket on the given port. Writing to it will return an error
// until a client connects.
func NewServerSocket(port int) Socket {
	conn, err := net.ListenPacket("udp", fmt.Sprintf("127.0.0.1:%v", port))
	if err != nil {
		fmt.Println("Couldn't create server on port:", port)
		return nil
	}
	conn.SetDeadline(time.Time{})
	read := make(chan []byte)
	sock := serverSocket{socketBase: socketBase{read}, conn: conn}

	// reader thread
	go func() {
		for {
			buf := make([]byte, 2048)
			n, addr, err := conn.ReadFrom(buf)
			if err != nil {
				fmt.Println("Couldn't read:", err)
				break
			}
			// catch handshake message for new clients
			// (behavior defined by UdpServer.cpp)
			isHandshake := addr != sock.addr && n == 1
			sock.addr = addr
			sock.ready = true
			if !isHandshake {
				read <- buf[:n]
			}
		}
	}()

	return &sock
}
