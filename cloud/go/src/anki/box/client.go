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

	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/google/uuid"
	"github.com/gwatts/rootcerts"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

var deviceID = "mac-build"

type client struct {
	ipc.Conn
}

var (
	defaultTLSCert = credentials.NewClientTLSFromCert(rootcerts.ServerCertPool(), "")
)

func (c *client) handleConn(ctx context.Context) {
	for {
		msgbuf := c.ReadBlock()
		if msgbuf == nil || len(msgbuf) == 0 {
			return
		}
		var msg vision.OffboardImageReady
		if err := msg.Unpack(bytes.NewBuffer(msgbuf)); err != nil {
			log.Println("Could not unpack box request:", err)
			continue
		}

		resp, err := c.handleRequest(ctx, &msg)
		if err != nil {
			log.Println("Error handling box request:", err)
		}
		if resp == nil {
			resp = &vision.OffboardResultReady{}
		}

		var buf bytes.Buffer
		if err := resp.Pack(&buf); err != nil {
			log.Println("Error packing box response:", err)
		} else if n, err := c.Write(buf.Bytes()); n != buf.Len() || err != nil {
			log.Println("Error sending box response:", fmt.Sprintf("%d/%d,", n, buf.Len()), err)
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

	sessionID := uuid.New().String()[:16]
	r := &pb.ImageRequest{
		Session:   sessionID,
		DeviceId:  deviceID,
		Lang:      "en",
		ImageData: fileData,
	}

	client := pb.NewChipperGrpcClient(rpcConn)
	resp, err := client.AnalyzeImage(ctx, r)
	if err != nil {
		log.Println("image analysis error: ", err)
		return nil, err
	}
	log.Println("image analysis response: ", resp.String())

	return &vision.OffboardResultReady{JsonResult: resp.RawResult}, nil
}
