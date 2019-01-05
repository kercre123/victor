package box

import (
	"anki/config"
	"anki/ipc"
	"anki/log"
	"anki/util"
	"bytes"
	"clad/vision"
	"context"
	"fmt"
	"io/ioutil"

	"github.com/gwatts/rootcerts"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

type client struct {
	ipc.Conn
}

var (
	defaultTLSCert = credentials.NewClientTLSFromCert(rootcerts.ServerCertPool(), "")
)

func (c *client) handleConn(ctx context.Context) {
	for {
		buf := c.ReadBlock()
		if buf == nil || len(buf) == 0 {
			return
		}
		var msg vision.OffboardImageReady
		if err := msg.Unpack(bytes.NewBuffer(buf)); err != nil {
			log.Println("Could not unpack box request:", err)
			continue
		}

		resp, err := c.handleRequest(ctx, &msg)
		if err != nil {
			log.Println("Error handling box request:", err)
		}
		if resp != nil {
			var buf bytes.Buffer
			if err := resp.Pack(&buf); err != nil {
				log.Println("Error packing box response:", err)
			} else if n, err := c.Write(buf.Bytes()); n != buf.Len() || err != nil {
				log.Println("Error sending box response:", fmt.Sprintf("%d/%d,", n, buf.Len()), err)
			}
		}
	}
}

func (c *client) handleRequest(ctx context.Context, msg *vision.OffboardImageReady) (*vision.OffboardResultReady, error) {
	var dialOpts []grpc.DialOption
	dialOpts = append(dialOpts, util.CommonGRPC()...)
	dialOpts = append(dialOpts, grpc.WithTransportCredentials(defaultTLSCert))

	var wg util.SyncGroup

	// dial server and read file data in parallel
	var rpcConn *grpc.ClientConn
	var rpcErr error
	rpcClose := func() error { return nil }

	var fileData []byte
	var fileErr error

	// dial server, make it blocking
	wg.AddFunc(func() {
		rpcConn, rpcErr = grpc.DialContext(ctx, config.Env.Box, append(dialOpts, grpc.WithBlock())...)
		if rpcErr == nil {
			rpcClose = rpcConn.Close
		}
	})

	// read file data
	wg.AddFunc(func() {
		fileData, fileErr = ioutil.ReadFile(msg.Filename)
	})

	// wait for both routines above to finish
	wg.Wait()
	// if rpc connection didn't fail, this will be set to rpcConn.Close()
	defer rpcClose()

	err := util.NewErrors(rpcErr, fileErr).Error()
	if err != nil {
		return nil, err
	}

	// todo: instantiate grpc client
	// client := pb.NewBoxClient(rpcConn)
	// client.ProtoOperation(request)
	// ...return result
	return &vision.OffboardResultReady{JsonResult: "[]"}, nil
}
