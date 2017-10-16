package main

import (
	"fmt"
	"net"
	"time"
)

// Socket
type Socket struct {
	write chan<- []byte
	read  <-chan []byte
}

// NewSocket
func NewClientSocket(ip string, port int) *Socket {
	conn, err := net.Dial("udp", fmt.Sprintf("%v:%v", ip, port))
	if err != nil {
		fmt.Println("Couldn't connect:", err)
		return nil
	}
	conn.SetDeadline(time.Time{})
	write, read := make(chan []byte), make(chan []byte)
	sock := Socket{write, read}
	done := make(chan struct{})

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
		done <- struct{}{}
	}()

	// writer thread
	go func() {
		for {
			buf := <-write
			_, err := conn.Write(buf)
			if err != nil {
				fmt.Println("Couldn't write:", err)
				break
			}
		}
		done <- struct{}{}
	}()

	// close connection on termination
	go func() {
		for i := 0; i < 2; i++ {
			<-done
		}
		conn.Close()
	}()

	return &sock
}

type ServerSocket struct {
	Socket
	addr  net.Addr
	ready bool
}

// NewServerSocket
func NewServerSocket(port int) *ServerSocket {
	conn, err := net.ListenPacket("udp", fmt.Sprintf("127.0.0.1:%v", port))
	if err != nil {
		fmt.Println("Couldn't create server on port:", port)
		return nil
	}
	conn.SetDeadline(time.Time{})
	write, read := make(chan []byte), make(chan []byte)
	sock := ServerSocket{Socket: Socket{write: write, read: read}}
	done := make(chan struct{})

	// reader thread
	go func() {
		for {
			buf := make([]byte, 2048)
			n, addr, err := conn.ReadFrom(buf)
			if err != nil {
				fmt.Println("Couldn't read:", err)
				break
			}
			sock.addr = addr
			sock.ready = true
			read <- buf[:n]
		}
		done <- struct{}{}
	}()

	// writer thread
	go func() {
		for {
			buf := <-write
			if !sock.ready {
				fmt.Println("Can't send before connection is established")
				break
			}
			_, err := conn.WriteTo(buf, sock.addr)
			if err != nil {
				fmt.Println("Couldn't write:", err)
				break
			}
		}
		done <- struct{}{}
	}()

	// close connection on termination
	go func() {
		for i := 0; i < 2; i++ {
			<-done
		}
		conn.Close()
	}()

	return &sock
}
