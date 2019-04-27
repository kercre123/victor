package svc

import (
	"context"
	"errors"
	"time"

	"github.com/aalpern/go-metrics"
	"github.com/gwatts/rootcerts"
	"github.com/jawher/mow.cli"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/metricsutil"
)

func init() {
	rootcerts.UpdateDefaultTransport()
}

type contextKey int

const serviceKey contextKey = iota

// WithService returns a derived context with the supplied pointer to
// a Service instance bound as a value.
func WithService(ctx context.Context, s *Service) context.Context {
	return context.WithValue(ctx, serviceKey, s)
}

// GetService extracts and returns the service pointer bound as a
// context value, or nil if no service pointer was found in the
// supplied context.
func GetService(ctx context.Context) *Service {
	if s, ok := ctx.Value(serviceKey).(*Service); ok {
		return s
	}
	return nil
}

// Service defines the central application object for a persistent
// service process. The Service instance is responsible for the
// overall control flow of startup and shutdown, handling command line
// arguments and environment variable configuration, invoking
// commands, and assorted bookkeeping.
//
// Service provides the following functionality:
//
//   * Configure and initialize logging (from command line options &
//     environment variables)
//   * Sets up signal handlers for ordered shut down
//   * Periodically log Go runtime and Kinesis logging stats
//   * Launch the profile server to enable metrics, runtime, and
//     health check collection
type Service struct {
	*cli.Cli

	// Name is an identifier for the service, which will be used to
	// initialize the root metrics registry and identify the service
	// in logging.
	Name string

	// Metrics is a metrics registry that should be used for
	// registering any service-specific metrics. Its prefix is
	// "service.<service-name>". Metrics will show up in Wavefront as
	// "go.service.<service-name>.path.to.metric"
	Metrics metrics.Registry

	// Global is a service object that is initialized for all commands
	// in the service. Any common sub-components, such as logging,
	// belong here.
	Global ServiceObject

	exit chan int
}

var errorServiceObjectNotSet = errors.New("Service object not set")

func (s *Service) CommandSpec() string {
	if s.Global != nil {
		if ci, ok := s.Global.(CommandInitializer); ok {
			return ci.CommandSpec()
		}
	}
	return ""
}

func (s *Service) CommandInitialize(cmd *cli.Cmd) {
	if s.Global != nil {
		if ci, ok := s.Global.(CommandInitializer); ok {
			ci.CommandInitialize(cmd)
		}
	}
}

func (s *Service) Start(ctx context.Context) error {
	if s.Global == nil {
		return errorServiceObjectNotSet
	} else {
		s.exit = make(chan int)
		alog.Info{
			"action": "service_start",
			"name":   s.Name,
			"status": alog.StatusStart,
		}.Log()
		if err := s.Global.Start(WithService(ctx, s)); err != nil {
			alog.Error{
				"action": "service_start",
				"status": alog.StatusError,
				"error":  err,
			}.Log()
			Fail("%s", err)
		}
	}
	return nil
}

func (s *Service) Exit(exitCode int) {
	if s.exit == nil {
		cli.Exit(exitCode)
	} else {
		alog.Debug{
			"action": "service_exit",
			"msg":    "Signalling to exit channel",
			"code":   exitCode,
		}.Log()
		s.exit <- exitCode
	}
}

func (s *Service) Stop() error {
	if s.Global == nil {
		return errorServiceObjectNotSet
	} else {
		alog.Info{
			"action": "service_stop",
			"name":   s.Name,
			"status": alog.StatusStart,
		}.Log()
		defer func() {
			alog.Info{
				"action": "service_stop",
				"name":   s.Name,
				"status": alog.StatusDone,
			}.Log()
		}()
		return s.Global.Stop()
	}
}

type ServiceConfig func(s *Service) error

func (s *Service) applyConfig(cfgs []ServiceConfig) error {
	for _, cfg := range cfgs {
		if err := cfg(s); err != nil {
			return err
		}
	}
	return nil
}

// NewService constructs a new ServiceApp object with the given name and
// configurations. Logging options, common commands, and bookkeeping
// routines will be set up automatically.
func NewService(serviceName, serviceDescription string, config ...ServiceConfig) (*Service, error) {
	svc := &Service{
		Name:    serviceName,
		Metrics: metricsutil.NewRegistry("service." + serviceName),
		Cli:     cli.App(serviceName, serviceDescription),
	}

	// apply the default service config
	if err := svc.applyConfig(DefaultServiceConfig); err != nil {
		return nil, err
	}

	// override with passed-in configuration.
	if err := svc.applyConfig(config); err != nil {
		return nil, err
	}

	svc.Spec = svc.CommandSpec()
	svc.CommandInitialize(svc.Cli.Cmd)

	// Add Before handler to initialize global service object
	svc.Before = func() {
		if err := svc.Start(context.Background()); err != nil {
			alog.Error{
				"action": "service_global_start",
				"status": alog.StatusError,
				"error":  err,
			}.Log()
			Fail("%s", err)
		}
	}

	// Add After handler to shut down global service object
	svc.After = func() {
		alog.Debug{
			"action": "service_global_stop",
			"status": alog.StatusStart,
		}.Log()
		if err := svc.Stop(); err != nil {
			alog.Warn{
				"action": "service_global_stop",
				"status": alog.StatusError,
				"error":  err,
			}.Log()
		}
		alog.Debug{
			"action": "service_global_stop",
			"status": alog.StatusDone,
		}.Log()

	}

	return svc, nil
}

// WithGlobal configures the global service object, which is
// initialize for all commands.
func WithGlobal(so ServiceObject) ServiceConfig {
	return func(s *Service) error {
		s.Global = so
		return nil
	}
}

// WithServiceCommand creates a new command and binds it to the
// supplied ServiceObject. The command wraps invocation of the
// ServiceObject's Start() method with handlers to perform an ordered
// shutdown when requested (for example, by a signal handler
// component) and to override the ordered shutdown by calling Kill()
// after a configurable timeout.
//
// The Start() method is launched in a separate go routine, while the
// main go routine waits for an exit code on a shutdown channel. It
// does not matter if the supplied ServiceObject's Start() method
// blocks or not.
func WithServiceCommand(name, description string, handler ServiceObject) ServiceConfig {
	return func(s *Service) error {
		s.Command(name, description, func(cmd *cli.Cmd) {
			if ci, ok := handler.(CommandInitializer); ok {
				cmd.Spec = ci.CommandSpec()
				ci.CommandInitialize(cmd)
			}

			cmd.Spec += " [--kill-timeout]"
			killTimeout := DurationOpt(cmd, "kill-timeout", 30*time.Second,
				"Time to allow for ordered shutdown before killing connections")

			cmd.Action = func() {
				alog.Info{
					"action": "service_command",
					"name":   name,
					"status": alog.StatusStart,
				}.Log()
				go func() {
					if err := handler.Start(WithService(context.Background(), s)); err != nil {
						alog.Error{
							"action":  "command_start",
							"status":  alog.StatusError,
							"command": name,
							"error":   err,
						}.Log()
						s.Exit(-1)
					}
				}()
				exitCode := <-s.exit
				alog.Info{
					"action":    "service_command",
					"name":      name,
					"status":    alog.StatusDone,
					"exit_code": exitCode,
				}.Log()
				Exit(exitCode)
			}

			cmd.After = func() {
				alog.Debug{
					"action":       "service_command_after",
					"name":         name,
					"status":       alog.StatusStart,
					"kill_timeout": *killTimeout,
				}.Log()

				stop := make(chan interface{})
				go func() {
					if err := handler.Stop(); err != nil {
						alog.Error{
							"action":  "command_stop",
							"status":  alog.StatusError,
							"command": name,
							"error":   err,
						}.Log()
					}
					close(stop)
				}()

				select {
				case <-stop:
					alog.Debug{
						"action": "service_command_after",
						"name":   name,
						"status": alog.StatusDone,
					}.Log()
				case <-time.After(killTimeout.Duration()):
					alog.Debug{
						"action": "service_command_after",
						"name":   name,
						"status": "timeout",
						"msg":    "Stop() timed out. Calling Kill()",
					}.Log()
					handler.Kill()
				}
			}
		})
		return nil
	}
}

var (
	DefaultServiceConfig = []ServiceConfig{
		WithGlobal(NewServiceObject("core",
			WithComponent(&LogComponent{}),
			WithComponent(&ProfilerComponent{}),
			WithShutdownSignalWatcher(),
			WithMemoryProfileDumpSignalWatcher(),
			WithRuntimeLogger(),
		)),
		WithVersionCommand(),
	}
)
