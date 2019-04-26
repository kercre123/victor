package chipperfn

import (
	"context"

	"github.com/jawher/mow.cli"

	"google.golang.org/grpc"

	"errors"
	"github.com/anki/sai-chipper-fn/proto/chipperfnpb"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/grpc/grpcclient"
	"github.com/anki/sai-service-framework/svc"
)

type Client interface {
	chipperfnpb.ChipperFnClient
	//interceptors.ServerInterceptorFactory // TODO
	svc.CommandInitializer
	svc.ServiceObject
}

type client struct {
	chipperfnpb.ChipperFnClient

	enable *bool

	// conn is grpc.Dial created during Start(). Need to close in Stop()
	conn *grpc.ClientConn

	// default gRPC client config
	cfg *grpcclient.ClientConfig
}

type ClientOpt func(*client)

func NewClient(opts ...ClientOpt) Client {
	c := &client{
		cfg: grpcclient.NewClientConfig("chipperfn"),
	}

	for _, opt := range opts {
		opt(c)
	}
	return c
}

func (c *client) CommandSpec() string {
	return c.cfg.CommandSpec() + " --chipperfn-enable"
}

func (c *client) CommandInitialize(cmd *cli.Cmd) {
	c.cfg.CommandInitialize(cmd)
	c.enable = svc.BoolOpt(cmd, "chipperfn-enable", true, "If set to true, chipperfn is enabled")
}

func (c *client) Start(ctx context.Context) error {
	if !*c.enable {
		alog.Warn{
			"action": "chipperfn_start",
			"status": "not_enabled",
		}.Log()
		return errors.New("chipperfn is not enabled")
	}

	if c.conn == nil {
		conn, err := c.cfg.Dial()
		if err != nil {
			return err
		}
		c.conn = conn
	}

	if c.ChipperFnClient == nil {
		c.ChipperFnClient = chipperfnpb.NewChipperFnClient(c.conn)
	}

	alog.Info{
		"action": "chipperfn_client_start",
		"status": "connected",
	}.Log()

	return nil
}

func (c *client) Stop() error {
	if c.conn != nil {
		err := c.conn.Close()
		if err != nil {
			return err
		}
		c.conn = nil
	}
	return nil
}

func (c *client) Kill() error {
	return c.Stop()
}

//
// Interceptors (None yet)
//
