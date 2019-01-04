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
	rpcConn, err := grpc.DialContext(ctx, config.Env.Box, dialOpts...)
	if err != nil {
		return nil, err
	}
	defer rpcConn.Close()

	// todo: instantiate grpc client
	// client := pb.NewBoxClient(rpcConn)
	// client.ProtoOperation(request)
	// ...return result
	return &vision.OffboardResultReady{JsonResult: "[]"}, nil
}
