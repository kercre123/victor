package ipc

import (
	"anki/util"
	"fmt"
	"io"
	"net"
	"sync"
)

type netListenerLike interface {
	Accept() (io.ReadWriteCloser, error)
	Close() error
}

// this is necessary to convert a net.Listener to a netListenerLike
type listenerWrapper struct {
	net.Listener
}

func (l listenerWrapper) Accept() (io.ReadWriteCloser, error) {
	return l.Listener.Accept()
}

type baseServer struct {
	listen   netListenerLike
	kill     chan struct{}
	conns    []Conn
	newConns chan Conn
	wg       sync.WaitGroup
}

func newBaseServer(listen netListenerLike, connWrapper func(io.ReadWriteCloser) io.ReadWriteCloser) (Server, error) {
	serv := &baseServer{listen, make(chan struct{}), make([]Conn, 0, 8), make(chan Conn), sync.WaitGroup{}}

	// start listen routine - adds new connections to array
	serv.wg.Add(1)
	go func() {
		defer serv.wg.Done()
		for {
			conn, err := serv.listen.Accept()
			if connWrapper != nil {
				conn = connWrapper(conn)
			}
			if err != nil {
				// end of life errors are expected if we've been killed
				if !util.CanSelect(serv.kill) {
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
func (serv *baseServer) Close() error {
	close(serv.kill)
	err := serv.listen.Close()
	serv.wg.Wait()
	for _, conn := range serv.conns {
		conn.Close()
	}
	close(serv.newConns)
	return err
}

// NewConns returns a channel that will be passed new clients upon connection to the server,
// until the server is closed
func (serv *baseServer) NewConns() <-chan Conn {
	return serv.newConns
}
