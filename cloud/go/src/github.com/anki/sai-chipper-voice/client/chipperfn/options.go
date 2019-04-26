package chipperfn

import (
	"google.golang.org/grpc"

	"github.com/anki/sai-chipper-fn/proto/chipperfnpb"
	"github.com/anki/sai-service-framework/grpc/grpcclient"
)

func WithClientConn(conn *grpc.ClientConn) ClientOpt {
	return func(c *client) {
		c.ChipperFnClient = chipperfnpb.NewChipperFnClient(conn)
	}
}

func WithConfig(cfg *grpcclient.ClientConfig) ClientOpt {
	return func(c *client) {
		c.cfg = cfg
	}
}
