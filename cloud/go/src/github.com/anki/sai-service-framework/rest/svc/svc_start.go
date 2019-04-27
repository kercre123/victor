package restsvc

import (
	"context"
	"errors"

	"github.com/jawher/mow.cli"

	"github.com/anki/sai-go-util/http/apirouter"
	"github.com/anki/sai-service-framework/svc"
)

// APIServerComponent is an adapter to run our HTTP API service code
// in the new service framework. It's more a proof-of-concept than
// production-ready, as the code in apirouter pretty much assumes that
// it's in control of the world, and running within the `svc` package
// framework may result in some redundancies.
type APIServerComponent struct {
	port *int
	cmd  *apirouter.StartCommand
}

func (c *APIServerComponent) CommandSpec() string {
	return "[--port]"
}

func (c *APIServerComponent) CommandInitialize(cmd *cli.Cmd) {
	c.port = svc.IntOpt(cmd, "http-port", 0,
		"The port number for the service to listen on")
}

func (c *APIServerComponent) Start(ctx context.Context) error {
	if *c.port == 0 {
		return errors.New("HTTP_PORT not set")
	}

	c.cmd = &apirouter.StartCommand{
		PortNum: *c.port,
	}
	c.cmd.Run([]string{})
	return nil
}

func (c *APIServerComponent) Stop() error {
	// StartCommand sets up its own signal handler to shut down
	return nil
}

func (c *APIServerComponent) Kill() error {
	// StartCommand sets up its own signal handler to shut down
	return nil
}
