package token

import (
	"anki/ipc"
	"anki/log"
	"anki/token/jwt"
	"bytes"
	"clad/cloud"
	"fmt"
)

// Run starts the token service for other code/processes to connect to and
// request tokens
func Run(optionValues ...Option) {
	var opts options
	for _, o := range optionValues {
		o(&opts)
	}

	if err := jwt.Init(); err != nil {
		log.Println("Error initializing jwt store:", err)
		return
	}

	if err := queueInit(opts.stop); err != nil {
		log.Println("Error initializing request queue:", err)
		return
	}

	initRefresher(opts.stop)

	if opts.server {
		serv, err := initServer(opts.stop)
		if err != nil {
			log.Println("Error creating token server:", err)
			return
		}

		for c := range serv.NewConns() {
			go handleConn(c)
		}
	}
	// if server isn't requested, our background routines will handle requests
	// and there's no need for this function to block
}

// HandleRequest will process the given request and return a response. It may block,
// either due to waiting for other requests to process or due to waiting for gRPC.
func HandleRequest(req *cloud.TokenRequest) (*cloud.TokenResponse, error) {
	return handleRequest(req)
}

func handleConn(c ipc.Conn) {
	for {
		buf := c.ReadBlock()
		// TODO: will this ever close?
		if buf == nil || len(buf) == 0 {
			return
		}
		var msg cloud.TokenRequest
		if err := msg.Unpack(bytes.NewBuffer(buf)); err != nil {
			log.Println("Could not unpack token request:", err)
			continue
		}

		resp, err := handleRequest(&msg)
		if err != nil {
			log.Println("Error handling token request:", err)
		}
		if resp != nil {
			var buf bytes.Buffer
			if err := resp.Pack(&buf); err != nil {
				log.Println("Error packing token response:", err)
			} else if n, err := c.Write(buf.Bytes()); n != buf.Len() || err != nil {
				log.Println("Error sending token response:", fmt.Sprintf("%d/%d,", n, buf.Len()), err)
			}
		}
	}
}

func handleRequest(m *cloud.TokenRequest) (*cloud.TokenResponse, error) {
	req := request{m: m, ch: make(chan *response)}
	defer close(req.ch)
	queue <- req
	resp := <-req.ch
	return resp.resp, resp.err
}

func initServer(stop <-chan struct{}) (ipc.Server, error) {
	serv, err := ipc.NewUnixgramServer(ipc.GetSocketPath("token_server"))
	if err != nil {
		return nil, err
	}

	if stop != nil {
		go func() {
			<-stop
			if err := serv.Close(); err != nil {
				log.Println("error closing token server:", err)
			}
		}()
	}
	return serv, nil
}
