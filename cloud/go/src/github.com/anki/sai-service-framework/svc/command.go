package svc

import (
	"github.com/jawher/mow.cli"
)

// CommandInitializer defines an interface for service objects that
// can initialize a command line command object for configuration
// options.
type CommandInitializer interface {

	// CommandSpec returns the syntax string for the options necessary
	// to configure the service object, if any.
	CommandSpec() string

	// CommandInitialize configures a mow.cli command object with the
	// options necessary to configure the service object, if any.
	CommandInitialize(cmd *cli.Cmd)
}

// WithCommand adds a command to the command line options based on a
// raw initializer function.
func WithCommand(name, description string, init cli.CmdInitializer) ServiceConfig {
	return func(s *Service) error {
		s.Command(name, description, init)
		return nil
	}
}

// WithCommandObject adds a command to the command line options based
// on an object that implements the CommandInitializer interface.
func WithCommandObject(name, description string, init CommandInitializer) ServiceConfig {
	return WithCommand(name, description, func(cmd *cli.Cmd) {
		cmd.Spec = init.CommandSpec()
		init.CommandInitialize(cmd)
	})
}
