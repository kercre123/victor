package jdocs

import (
	"anki/log"
	"clad/cloud"
	"context"
	"fmt"

	pb "github.com/anki/sai-jdocs/proto/jdocspb"
	"github.com/gwatts/rootcerts"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

const jdocsURL = "jdocs-dev.api.anki.com:443"

type conn struct {
	conn   *grpc.ClientConn
	client pb.JdocsClient
}

var (
	platformOpts   []grpc.DialOption
	defaultTLSCert = credentials.NewClientTLSFromCert(rootcerts.ServerCertPool(), "")
)

func newConn(ctx context.Context, opts *options) (*conn, error) {
	var dialOpts []grpc.DialOption
	if platformOpts != nil && len(platformOpts) > 0 {
		dialOpts = append(dialOpts, platformOpts...)
	}
	dialOpts = append(dialOpts, grpc.WithTransportCredentials(defaultTLSCert))
	if opts.tokener != nil {
		creds, err := opts.tokener.Credentials()
		if err != nil {
			return nil, err
		}
		dialOpts = append(dialOpts, grpc.WithPerRPCCredentials(creds))
	}
	rpcConn, err := grpc.DialContext(ctx, jdocsURL, dialOpts...)
	if err != nil {
		return nil, err
	}

	rpcClient := pb.NewJdocsClient(rpcConn)

	ret := &conn{
		conn:   rpcConn,
		client: rpcClient}
	return ret, nil
}

func (c *conn) handleRequest(ctx context.Context, req *cloud.DocRequest) (*cloud.DocResponse, error) {
	switch req.Tag() {
	case cloud.DocRequestTag_Read:
		return c.readRequest(ctx, req.GetRead())
	case cloud.DocRequestTag_Write:
		return c.writeRequest(ctx, req.GetWrite())
	case cloud.DocRequestTag_DeleteReq:
		return c.deleteRequest(ctx, req.GetDeleteReq())
	case cloud.DocRequestTag_Echo:
		return c.echoRequest(ctx, req.GetEcho())
	}
	err := fmt.Errorf("Major error: received unknown tag %d", req.Tag())
	log.Println(err)
	return nil, err
}

var connectErrorResponse = cloud.NewDocResponseWithErr(&cloud.ErrorResponse{cloud.DocError_ErrorConnecting})

func (c *conn) writeRequest(ctx context.Context, cladReq *cloud.WriteRequest) (*cloud.DocResponse, error) {
	req := (*cladWriteReq)(cladReq).toProto()
	resp, err := c.client.WriteDoc(ctx, req)
	if err != nil {
		return connectErrorResponse, err
	}
	return cloud.NewDocResponseWithWrite((*protoWriteResp)(resp).toClad()), nil
}

func (c *conn) readRequest(ctx context.Context, cladReq *cloud.ReadRequest) (*cloud.DocResponse, error) {
	req := (*cladReadReq)(cladReq).toProto()
	resp, err := c.client.ReadDocs(ctx, req)
	if err != nil {
		return connectErrorResponse, err
	}
	return cloud.NewDocResponseWithRead((*protoReadResp)(resp).toClad()), nil
}

func (c *conn) deleteRequest(ctx context.Context, cladReq *cloud.DeleteRequest) (*cloud.DocResponse, error) {
	req := (*cladDeleteReq)(cladReq).toProto()
	_, err := c.client.DeleteDoc(ctx, req)
	if err != nil {
		return connectErrorResponse, err
	}
	return cloud.NewDocResponseWithDeleteResp(&cloud.Void{}), nil
}

func (c *conn) echoRequest(ctx context.Context, cladReq *cloud.DocEcho) (*cloud.DocResponse, error) {
	req := &pb.EchoReq{Data: cladReq.Data}
	resp, err := c.client.Echo(ctx, req)
	if err != nil {
		return connectErrorResponse, err
	}
	return cloud.NewDocResponseWithEcho(&cloud.DocEcho{Data: resp.Data}), nil
}
