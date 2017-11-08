package ipc

import (
	"errors"
	"fmt"
	"net"
	"time"
)

const (
	maxPacketSize     = 1024 * 2
	readTimeoutLength = 50 * time.Millisecond
)

// Socket provides an interface for inter-process communication via sockets.
// Servers are assumed to be connected to only one client, so writing doesn't have
// any notion of a destination; clients send messages to servers, and servers send
// messages back to the client that connected to them.
type Socket interface {
	Read() (n int, b []byte)           // Reads data from the socket if available, otherwise returns (0, nil)
	ReadBlock() (n int, b []byte)      // Reads data from the socket, blocking until data becomes available
	Write(b []byte) (n int, err error) // Write a message to the socket
	Close() error                      // Close socket and stop associated goroutines
}

type socketBase struct {
	read  chan []byte
	close chan<- struct{}
}

type clientSocket struct {
	socketBase
	conn net.Conn
}

func (s *socketBase) Read() (n int, b []byte) {
	select {
	case b = <-s.read:
	default:
		b = make([]byte, 0)
	}
	n = len(b)
	return
}

func (s *socketBase) ReadBlock() (n int, b []byte) {
	b = <-s.read
	n = len(b)
	return
}

func (s *clientSocket) Write(b []byte) (n int, err error) {
	return s.conn.Write(b)
}

func (s *clientSocket) Close() error {
	s.close <- struct{}{}
	return s.conn.Close()
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
	close := make(chan struct{})
	sock := clientSocket{socketBase{read, close}, conn}

	// Send one byte to server so it marks us as connected
	// (behavior defined by UdpClient.cpp)
	conn.Write(make([]byte, 1))

	// reader thread
	readFunc := func(buf []byte) (int, error) {
		return conn.Read(buf)
	}
	go readerRoutine(conn, read, readFunc, close)

	return &sock, nil
}

type serverSocket struct {
	socketBase
	conn  net.PacketConn
	addr  net.Addr
	ready bool
}

func (s *serverSocket) Write(b []byte) (n int, err error) {
	if !s.ready {
		return 0, errors.New("Can't write to unconnected server socket")
	}
	return s.conn.WriteTo(b, s.addr)
}

func (s *serverSocket) Close() error {
	s.close <- struct{}{}
	return s.conn.Close()
}

type deadliner interface {
	SetReadDeadline(t time.Time) error
}

func readerRoutine(conn deadliner, read chan<- []byte, readFunc func([]byte) (int, error), close <-chan struct{}) {
	buf := make([]byte, maxPacketSize)
readloop:
	for {
		// see if we've gotten shutdown notification
		select {
		case <-close:
			break readloop
		default:
		}
		conn.SetReadDeadline(time.Now().Add(readTimeoutLength))
		n, err := readFunc(buf)
		if err != nil {
			if neterr, ok := err.(net.Error); ok && neterr.Timeout() {
				// do nothing on timeout
				continue
			}
			fmt.Println("Socket couldn't read:", err)
			break
		}
		if n == cap(buf) {
			fmt.Println("Potential socket overload")
		}
		// catch handshake message for new clients
		// (behavior defined by UdpServer.cpp)
		if n > 0 {
			retbuf := make([]byte, n)
			copy(retbuf, buf)
			read <- retbuf
		}
	}
}

// NewServerSocket returns a new server socket on the given port. Writing to it will return an error
// until a client connects.
func NewServerSocket(port int) (Socket, error) {
	conn, err := net.ListenPacket("udp", fmt.Sprintf("localhost:%v", port))
	if err != nil {
		return nil, err
	}
	conn.SetDeadline(time.Time{})
	read := make(chan []byte)
	close := make(chan struct{})
	sock := serverSocket{socketBase: socketBase{read, close}, conn: conn}

	readFunc := func(buf []byte) (int, error) {
		n, addr, err := conn.ReadFrom(buf)
		if err == nil {
			// catch handshake message for new clients
			// (behavior defined by UdpServer.cpp)
			sameAddress := addr != nil && sock.addr != nil && addr.String() == sock.addr.String()
			isHandshake := !sameAddress && n == 1
			sock.addr = addr
			sock.ready = true
			if isHandshake {
				// don't send data onward; setting received bytes to 0 will stop this
				n = 0
			}
		}
		return n, err
	}
	go readerRoutine(conn, read, readFunc, close)

	return &sock, nil
}
