package grpcclient

import (
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/svc"

	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
)

// ClientCommand provides a basis for building CLI commands in other
// project (such as sai-go-cli) that involve connecting to a gRPC
// service. It handles all necessary gRPC configuration options and
// connection establishment, as well as loading a standard set of
// interceptors to provide logging and metrics.
type ClientCommand struct {

	// Action is a handler function that will be called by the command
	// object once the gRPC connection has been established.
	Action func(conn *grpc.ClientConn) error

	Spec string

	Init func(*cli.Cmd)

	cfg *ClientConfig
}

type ClientCommandAction func(conn *grpc.ClientConn) error

func NewClientCommand(prefix string, spec string, init func(*cli.Cmd), action ClientCommandAction) *ClientCommand {
	return &ClientCommand{
		Spec:   spec,
		Init:   init,
		Action: action,
		cfg:    NewClientConfig(prefix),
	}
}

func (c *ClientCommand) CommandSpec() string {
	return c.cfg.CommandSpec() + " " + c.Spec
}

func (c *ClientCommand) CommandInitialize(cmd *cli.Cmd) {
	c.cfg.CommandInitialize(cmd)
	if c.Init != nil {
		c.Init(cmd)
	}

	cmd.Action = func() {
		if c.Action == nil {
			panic("gRPC client command Action not set")
		}

		conn, err := c.cfg.Dial()
		if err != nil {
			svc.Fail("%s", err)
		}
		defer conn.Close()

		if err = c.Action(conn); err != nil {
			alog.Error{
				"action": "run",
				"status": "error",
				"error":  err,
			}.Log()
			svc.Fail("%s", err)
		}
	}
}
