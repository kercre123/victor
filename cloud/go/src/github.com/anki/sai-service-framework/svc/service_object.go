package svc

import (
	"context"
	"reflect"
	"strings"

	"github.com/anki/sai-go-util/log"
	"github.com/jawher/mow.cli"
)

// ServiceObject defines a basic interface for objects that have a
// startup/shutdown lifecycle.
type ServiceObject interface {
	Start(ctx context.Context) error
	Stop() error
	Kill() error
}

// SimpleServiceObject is an implementaton of both ServiceObject and
// CommandInitializer that can be constructed on the fly
type SimpleServiceObject struct {
	OnStart            func(context.Context) error
	OnStop             func() error
	OnKill             func() error
	Spec               string
	CommandInitializer cli.CmdInitializer
}

func (s *SimpleServiceObject) Start(ctx context.Context) error {
	if s.OnStart != nil {
		return s.OnStart(ctx)
	}
	return nil
}

func (s *SimpleServiceObject) Stop() error {
	if s.OnStop != nil {
		return s.OnStop()
	}
	return nil
}

func (s *SimpleServiceObject) Kill() error {
	if s.OnKill != nil {
		return s.OnKill()
	}
	return nil
}

func (s *SimpleServiceObject) CommandSpec() string {
	return s.Spec
}

func (s *SimpleServiceObject) CommandInitialize(cmd *cli.Cmd) {
	if s.CommandInitializer != nil {
		s.CommandInitializer(cmd)
	}
}

// CompositeServiceObject is an implementation of ServiceObject and
// CommandInitializer for composing multiple service objects
// together.
type CompositeServiceObject struct {
	Name       string
	components []ServiceObject
}

func (s *CompositeServiceObject) CommandSpec() string {
	spec := ""
	for _, c := range s.components {
		if ci, ok := c.(CommandInitializer); ok {
			spec = spec + " " + ci.CommandSpec()
		}
	}
	return strings.TrimSpace(spec)
}

func (s *CompositeServiceObject) CommandInitialize(cmd *cli.Cmd) {
	for _, c := range s.components {
		if ci, ok := c.(CommandInitializer); ok {
			ci.CommandInitialize(cmd)
		}
	}
}

func (s *CompositeServiceObject) Start(ctx context.Context) error {
	alog.Debug{
		"action":          "composite_service_object_start",
		"status":          alog.StatusStart,
		"name":            s.Name,
		"component_count": len(s.components),
	}.Log()
	for i, c := range s.components {
		alog.Debug{
			"action":           "composite_service_object_start",
			"status":           "subcomponent_start",
			"name":             s.Name,
			"component_number": i,
			"component_type":   reflect.TypeOf(c),
		}.Log()
		if err := c.Start(ctx); err != nil {
			return err
		}
	}
	return nil
}

func (s *CompositeServiceObject) Stop() error {
	alog.Debug{
		"action":          "service_object_stop",
		"status":          alog.StatusStart,
		"name":            s.Name,
		"component_count": len(s.components),
	}.Log()
	for i, c := range s.components {
		defer func(so ServiceObject) {
			if err := so.Stop(); err != nil {
				alog.Warn{
					"action":           "service_object_stop",
					"name":             s.Name,
					"status":           alog.StatusError,
					"component_number": i,
					"component_type":   reflect.TypeOf(so),
					"error":            err,
				}.Log()
			}
		}(c)
	}
	alog.Debug{
		"action": "service_object_stop",
		"status": alog.StatusOK,
		"name":   s.Name,
	}.Log()
	return nil
}

func (s *CompositeServiceObject) Kill() error {
	alog.Debug{
		"action":          "service_object_kill",
		"status":          alog.StatusStart,
		"name":            s.Name,
		"component_count": len(s.components),
	}.Log()
	for i, c := range s.components {
		defer func(so ServiceObject) {
			if err := so.Kill(); err != nil {
				alog.Warn{
					"action":           "service_object_kill",
					"name":             s.Name,
					"status":           alog.StatusError,
					"component_number": i,
					"component_type":   reflect.TypeOf(so),
					"error":            err,
				}.Log()
			}
		}(c)
	}
	alog.Debug{
		"action": "service_object_kill",
		"status": alog.StatusOK,
		"name":   s.Name,
	}.Log()
	return nil
}

func (s *CompositeServiceObject) Add(component ServiceObject) {
	s.components = append(s.components, component)
}

//
// Functional configs
//

type ConfigFn func(s *CompositeServiceObject) error

func (s *CompositeServiceObject) applyConfigs(cfgs []ConfigFn) error {
	for _, cfg := range cfgs {
		if err := cfg(s); err != nil {
			return err
		}
	}
	return nil
}

func NewServiceObject(name string, configs ...ConfigFn) *CompositeServiceObject {
	obj := &CompositeServiceObject{
		Name: name,
	}
	if err := obj.applyConfigs(configs); err != nil {
		Fail("Error constructing service object '%s': %s", name, err)
	}
	return obj
}

func WithComponent(component ServiceObject) ConfigFn {
	return func(parent *CompositeServiceObject) error {
		parent.Add(component)
		return nil
	}
}
