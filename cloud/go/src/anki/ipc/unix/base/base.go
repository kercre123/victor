package base

import (
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"sync"
)

// Conn represents a connection between two specific network endpoints - a client
// will hold one Conn to the server it's connected to, a server will hold individual
// Conns for each client that's connected to it
type Conn interface {
	Read() []byte              // read a message (returns nil if nothing is available)
	ReadBlock() []byte         // read a message and block until one becomes available or the connection is closed
	Write([]byte) (int, error) // write the given buffer to the other end
	Close() error              // close the connection and associated resources
}

// baseConn wraps a net.Conn to manage the resources we associate with one (reader routines, channels)
type baseConn struct {
	conn net.Conn
	kill chan struct{}
	read chan []byte
	wg   sync.WaitGroup
}

func (c *baseConn) Close() error {
	close(c.kill)
	ret := c.conn.Close()
	c.wg.Wait() // wait for reader routine to close
	close(c.read)
	return ret
}

func (c *baseConn) Read() (b []byte) {
	select {
	case b = <-c.read:
	default:
		b = []byte{}
	}
	return
}

func (c *baseConn) ReadBlock() (b []byte) {
	select {
	case b = <-c.read:
	case <-c.kill:
	}
	return
}

func (c *baseConn) Write(b []byte) (int, error) {
	// first write the length, since unix sockets behave like streams but we're
	// sending discrete messages
	err := binary.Write(c.conn, binary.LittleEndian, uint32(len(b)))
	if err != nil {
		return 0, err
	}
	return c.conn.Write(b)
}

// quick helper for 'kill' channels - return if it's been closed or not
func canSelect(ch chan struct{}) bool {
	select {
	case <-ch:
		return true
	default:
		return false
	}
}

func newBaseConn(conn net.Conn) Conn {
	ret := &baseConn{conn, make(chan struct{}), make(chan []byte), sync.WaitGroup{}}

	// reader thread: puts incoming messages into read channel, waiting to be consumed
	ret.wg.Add(1)
	go func() {
		defer ret.wg.Done()
		var buflen uint32
		for {
			// corresponding with Write(), we first have to read the length
			// of the message we're pulling
			err := binary.Read(conn, binary.LittleEndian, &buflen)
			if err != nil {
				if err == io.EOF || canSelect(ret.kill) {
					// expect to get errors at end of life
					break
				} else {
					fmt.Println("Socket couldn't read length:", err)
					break
				}
			}

			sendbuf := make([]byte, buflen)
			n, err := conn.Read(sendbuf)
			if n != cap(sendbuf) {
				fmt.Printf("Expected length %v, got %v\n", buflen, n)
			} else if n == 0 {
				continue
			}
			select {
			case ret.read <- sendbuf:
			case <-ret.kill:
			}
		}
	}()

	return ret
}

// UnixServer is an instance of a domain socket server that will spawn new connections
// (listen via the channel NewConns() returns) when clients connect
type UnixServer struct {
	listen   net.Listener
	kill     chan struct{}
	conns    []Conn
	newConns chan Conn
	wg       sync.WaitGroup
}

// NewUnixServer returns a new server object listening for clients on the specified path,
// if no errors are encountered
func NewUnixServer(path string) (*UnixServer, error) {
	listen, err := net.Listen("unix", path)
	if err != nil {
		return nil, err
	}
	serv := &UnixServer{listen, make(chan struct{}), make([]Conn, 0, 8), make(chan Conn), sync.WaitGroup{}}

	// start listen routine - adds new connections to array
	serv.wg.Add(1)
	go func() {
		defer serv.wg.Done()
		for {
			conn, err := listen.Accept()
			if err != nil {
				// end of life errors are expected if we've been killed
				if !canSelect(serv.kill) {
					fmt.Println("Listen error:", err)
				}
				return
			}
			newConn := newBaseConn(conn)
			serv.conns = append(serv.conns, newConn)
			serv.wg.Add(1)
			go func() {
				defer serv.wg.Done()
				select {
				case serv.newConns <- newConn:
				case <-serv.kill:
				}
			}()
		}
	}()

	return serv, nil
}

// Close stops the server from listening and closes all existing connections
func (serv *UnixServer) Close() {
	close(serv.kill)
	serv.listen.Close()
	serv.wg.Wait()
	for _, conn := range serv.conns {
		conn.Close()
	}
	close(serv.newConns)
}

// NewConns returns a channel that will be passed new clients upon connection to the server,
// until the server is closed
func (serv *UnixServer) NewConns() <-chan Conn {
	return serv.newConns
}

// NewUnixClient returns a new connection to the server at the specified path, assuming
// there are no errors when connecting
func NewUnixClient(path string) (Conn, error) {
	conn, err := net.Dial("unix", path)
	if err != nil {
		fmt.Println("Dial error:", err)
		return nil, err
	}
	client := newBaseConn(conn)
	return client, nil
}
