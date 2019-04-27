package svc

import (
	"context"
	"errors"
	"net/http"
	"time"

	"github.com/anki/sai-go-util/http/statushandler"
	"github.com/jawher/mow.cli"
)

// StatusMonitorComponent provides a service framework hook into the
// functionality provided by the
// github.com/anki/sai-go-util/http/statushandler package, which helps
// services expose health checks that are periodically evaluated to
// determine whether the status of a service is OK or unhealthy.
//
// Status handlers are also exposed on the profile server port
// provided by the ProfileServerComponent.
type StatusMonitorComponent struct {
	frequency    *Duration
	timeout      *Duration
	triggerCount *int
	checker      statushandler.StatusChecker
	monitor      *statushandler.StatusMonitor
}

type StatusMonitorOption func(s *StatusMonitorComponent)

// WithStatusChecker sets the status checker used to determine the
// service's status, from an object implementing the
// statushandler.StatusChecker interface.
//
// At present only one status check is supported.
func WithStatusChecker(s statushandler.StatusChecker) StatusMonitorOption {
	return func(c *StatusMonitorComponent) {
		c.checker = s
	}
}

// WithStatusCheckerFunc sets the status checker used to determine the
// service's status, from any function that returns an error, or nil
// if all is well.
//
// At present only one status check is supported.
func WithStatusCheckerFunc(f func() error) StatusMonitorOption {
	return WithStatusChecker(statushandler.StatusCheckerFunc(f))
}

// WithStatusMonitor is a service object config option which adds a
// status monitor object to a composite service object with the
// provided status checker.
//
// At present only one status check is supported.
func WithStatusMonitor(checker statushandler.StatusChecker) ConfigFn {
	return WithComponent(NewStatusMonitorComponent(WithStatusChecker(checker)))
}

func NewStatusMonitorComponent(opts ...StatusMonitorOption) *StatusMonitorComponent {
	s := &StatusMonitorComponent{}
	for _, opt := range opts {
		opt(s)
	}
	return s
}

func (s *StatusMonitorComponent) CommandSpec() string {
	return "[--status-frequency] [--status-timeout] [--status-trigger-count]"
}

func (s *StatusMonitorComponent) CommandInitialize(cmd *cli.Cmd) {
	s.frequency = DurationOpt(cmd, "status-frequency", time.Second*10,
		"How often to poll the status checker")
	s.timeout = DurationOpt(cmd, "status-timeout", time.Second*10,
		"How long to wait before failing a status check")
	s.triggerCount = IntOpt(cmd, "status-trigger-count", 3,
		"Number of failures/successes needed to change health status")
}

func (s *StatusMonitorComponent) Start(ctx context.Context) error {
	if s.monitor != nil {
		return errors.New("Status monitor already started")
	}
	if s.checker == nil {
		return errors.New("Status monitor function not set")
	}
	http.Handle("/status", statushandler.StatusHandler{s.checker})
	s.monitor = statushandler.NewMonitorWithTimeout(s.frequency.Duration(),
		s.timeout.Duration(), *s.triggerCount, s.checker)
	return nil
}

func (s *StatusMonitorComponent) Stop() error {
	if s.monitor != nil {
		s.monitor.Stop()
	}
	return nil
}

func (s *StatusMonitorComponent) Kill() error {
	return s.Stop()
}
