package jdocs

import (
	"anki/ipc"
	"anki/log"
	"bytes"
	"clad/cloud"
	"context"
	"fmt"
	"sync"
)

func runServer(ctx context.Context, opts *options) {
	serv, err := ipc.NewUnixgramServer(ipc.GetSocketPath("jdocs_server"))
	if err != nil {
		log.Println("Error creating jdocs server:", err)
		return
	}

	// close on context?
	for c := range serv.NewConns() {
		cl := client{c, opts}
		go cl.handleConn(ctx)
	}
}

type client struct {
	ipc.Conn
	opts *options
}

func (c *client) handleConn(ctx context.Context) {
	for {
		buf := c.ReadBlock()
		// TODO: will this ever close?
		if buf == nil || len(buf) == 0 {
			return
		}
		var msg cloud.DocRequest
		if err := msg.Unpack(bytes.NewBuffer(buf)); err != nil {
			log.Println("Could not unpack jdocs request:", err)
			continue
		}

		resp, err := c.handleRequest(ctx, &msg)
		if err != nil {
			log.Println("Error handling jdocs request:", err)
		}
		if resp != nil {
			var buf bytes.Buffer
			if err := resp.Pack(&buf); err != nil {
				log.Println("Error packing jdocs response:", err)
			} else if n, err := c.Write(buf.Bytes()); n != buf.Len() || err != nil {
				log.Println("Error sending jdocs response:", fmt.Sprintf("%d/%d,", n, buf.Len()), err)
			}
		}
	}
}

var reqMutex sync.Mutex

func (c *client) handleRequest(ctx context.Context, msg *cloud.DocRequest) (*cloud.DocResponse, error) {
	reqMutex.Lock()
	defer reqMutex.Unlock()
	if ok, resp, err := c.handleConnectionless(msg); ok {
		return resp, err
	}
	conn, err := newConn(ctx, c.opts)
	if err != nil {
		return connectErrorResponse, err
	}
	defer conn.close()
	return conn.handleRequest(ctx, msg)
}
