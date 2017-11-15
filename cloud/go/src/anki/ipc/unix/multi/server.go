package multi

import (
	"anki/ipc/unix/base"
	"fmt"
	"strings"
	"sync"
)

// UnixServer is an instance of a server daemon routing messages between its connected clients
type UnixServer struct {
	base     *base.UnixServer
	kill     chan struct{}
	clients  map[string]*endpoint
	incoming chan *message
	wg       sync.WaitGroup
}

type endpoint struct {
	conn base.Conn
	name string
}

type message struct {
	src  *endpoint
	dest string
	buf  []byte
}

// NewUnixServer returns a server instance listening for new connections on the specified path
func NewUnixServer(path string) (*UnixServer, error) {
	baseServ, err := base.NewUnixServer(path)
	if err != nil {
		return nil, err
	}
	serv := &UnixServer{baseServ, make(chan struct{}), make(map[string]*endpoint), make(chan *message), sync.WaitGroup{}}

	// get the channel that new clients will be sent on
	endpoints := serv.handshakeRoutine()
	// pass it to the routine that will set up new clients with routines to listen for messages
	endpoints = serv.newEndpointRoutine(endpoints)
	// start the routine that will route messages to their destination clients
	serv.messageRoutine(endpoints)

	return serv, nil
}

// takes new connections, gets their client name, and sends the endpoint data (name+connection)
// to the "new endpoint" routine
func (serv *UnixServer) handshakeRoutine() <-chan *endpoint {
	ret := make(chan *endpoint)

	serv.wg.Add(1)
	go func() {
		defer serv.wg.Done()
		defer close(ret)

		for conn := range serv.base.NewConns() {
			go func(conn base.Conn) {
				buf := conn.ReadBlock()
				if len(buf) > 0 {
					ret <- &endpoint{conn, string(buf)}
				}
			}(conn)
		}
	}()

	return ret
}

// sets up listeners for new endpoints connecting to the server
func (serv *UnixServer) newEndpointRoutine(clients <-chan *endpoint) <-chan *endpoint {
	outClients := make(chan *endpoint)

	serv.wg.Add(1)
	go func() {
		defer serv.wg.Done()
		defer close(outClients)

		wg := sync.WaitGroup{}
		for client := range clients {
			// pass wait group - since we spawn the routines that will push data to the incoming
			// messages channel, we'll close it once all routines are done
			wg.Add(1)
			serv.endpointListener(client, &wg)
			outClients <- client
		}
		// wait for all endpoint routines to shut down, then close incoming channel
		wg.Wait()
		close(serv.incoming)
	}()

	return outClients
}

func (serv *UnixServer) endpointListener(client *endpoint, wg *sync.WaitGroup) {
	// this routine starts up once for every client
	go func() {
		defer wg.Done()
		for {
			buf := client.conn.ReadBlock()
			if buf == nil || len(buf) == 0 {
				return
			}

			// incoming messages will have the destination name, a null char, then message contents
			nullIdx := strings.Index(string(buf), "\x00")
			if nullIdx < 0 {
				fmt.Println("Error: couldn't find null separator in message")
				nullIdx = 0
			}
			dest := string(buf[:nullIdx])
			select {
			case <-serv.kill:
				return
			case serv.incoming <- &message{client, dest, buf[nullIdx+1:]}:
			}
		}
	}()
}

// message router - takes messages on incoming channel and sends them to their destination
// also adds new clients to client map
func (serv *UnixServer) messageRoutine(clients <-chan *endpoint) {
	serv.wg.Add(1)

	go func() {
		defer serv.wg.Done()
		for {
			select {
			case msg := <-serv.incoming:
				destClient, ok := serv.clients[msg.dest]
				if !ok {
					// todo: buffer this to send if client later connects
					fmt.Println("Unknown destination client", msg.dest)
					continue
				}
				destClient.conn.Write(getBufferForMessage(msg.src.name, msg.buf))

			case client := <-clients:
				// add new client to map so we can find it when someone sends to it
				serv.clients[client.name] = client

			case <-serv.kill:
				return
			}
		}
	}()
}

// Close stops the server and closes associated connections and resources
func (serv *UnixServer) Close() {
	serv.base.Close()
	close(serv.kill)
	serv.wg.Wait()
}
