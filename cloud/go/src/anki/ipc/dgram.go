package ipc

import (
	"anki/util"
	"errors"
	"fmt"
	"io"
	"net"
)

// for the UDP server, we need to construct a translation from
// the net.PacketConn returned by net.ListenPacket("udp", ...)
// to a net.Listener with semantics to return new connections
type packetListener struct {
	conn       net.PacketConn
	clients    map[string]*packetClient
	newClients chan *packetClient
	kill       chan struct{}
	closer     *util.DoOnce
}

func (p *packetListener) Accept() (io.ReadWriteCloser, error) {
	select {
	case conn := <-p.newClients:
		return conn, nil
	case <-p.kill:
		return nil, io.EOF
	}
}

func (p *packetListener) Close() error {
	p.closer.Do()
	return p.conn.Close()
}

type packetClient struct {
	conn net.PacketConn
	addr net.Addr
	read chan []byte
	kill chan struct{}
}

func (c *packetClient) Read(buf []byte) (int, error) {
	select {
	case <-c.kill:
		return 0, io.EOF
	case recv := <-c.read:
		if len(buf) < len(recv) {
			fmt.Println("overflow error", len(buf), len(recv))
		}
		return copy(buf, recv), nil
	}
}

func (c *packetClient) Close() error {
	// packetClient is closed by its server being closed; no-op
	return nil
}

func (c *packetClient) Write(buf []byte) (int, error) {
	return c.conn.WriteTo(buf, c.addr)
}

func newDatagramClient(conn net.Conn) (Conn, error) {
	client := newBaseConn(conn)

	// Send one byte to server so it marks us as connected
	// (behavior defined by UdpClient.cpp)
	n, err := conn.Write(make([]byte, 1))
	if err != nil {
		return nil, err
	} else if n != 1 {
		return nil, errors.New(fmt.Sprint("unexpected write size", n))
	}

	return client, nil
}

func newDatagramServer(conn net.PacketConn) (Server, error) {
	kill := make(chan struct{})
	closer := util.NewDoOnce(func() { close(kill) })
	newClients := make(chan *packetClient)
	listener := &packetListener{conn, make(map[string]*packetClient), newClients, kill, &closer}

	// start reader thread
	go func() {
		defer closer.Do()
		buf := make([]byte, 16384)
		for {
			if util.CanSelect(kill) {
				return
			}
			n, address, err := conn.ReadFrom(buf)
			if err != nil || address == nil {
				return
			}
			if n == len(buf) {
				fmt.Println("Maxed buffer")
			}
			addr := address.String()
			if client, ok := listener.clients[addr]; ok {
				sendbuf := make([]byte, n)
				copy(sendbuf, buf)
				select {
				case client.read <- sendbuf:
				case <-kill:
					return
				}
			} else if n == 1 {
				client = &packetClient{conn, address, make(chan []byte), kill}
				listener.clients[addr] = client
				select {
				case newClients <- client:
				case <-kill:
					return
				}
			} else {
				fmt.Println("unexpected buf size from unknown client:", n, addr)
			}
		}
	}()

	return newBaseServer(listener, nil)
}
