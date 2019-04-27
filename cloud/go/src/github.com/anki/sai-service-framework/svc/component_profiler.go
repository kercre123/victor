package svc

import (
	"context"
	"errors"
	"fmt"

	acli "github.com/anki/sai-go-util/cli"
	"github.com/anki/sai-go-util/log"
	"github.com/jawher/mow.cli"
)

// ProfilerComponent is a service object component which captures
// runtime profiling information when activated.
type ProfilerComponent struct {
	mode     *string
	path     *string
	profiler acli.Profiler
}

func (c *ProfilerComponent) CommandSpec() string {
	return "[--profile-mode] [--profile-path]"
}

func (c *ProfilerComponent) CommandInitialize(cmd *cli.Cmd) {
	c.mode = StringOpt(cmd, "profile-mode", "",
		`Capture profiling information. Can be set to "cpu", "mem" or "block"`)
	c.path = StringOpt(cmd, "profile-path", "/tmp",
		"Directory to write profiling output to")
}

func (c *ProfilerComponent) Start(ctx context.Context) error {
	if *c.mode != "" {
		c.profiler = acli.StartProfiler(*c.path, *c.mode)
		if c.profiler == nil {
			return errors.New(fmt.Sprintf(`Unknown profile mode "%s"`, *c.mode))
		}
		alog.Debug{
			"action": "service_object_start",
			"status": alog.StatusOK,
			"name":   "core.profiler",
			"mode":   *c.mode,
			"path":   *c.path,
		}.Log()
	}
	return nil
}

func (c *ProfilerComponent) Stop() error {
	if c.profiler != nil {
		c.profiler.Stop()
	}
	return nil
}

func (c *ProfilerComponent) Kill() error {
	return c.Stop()
}
