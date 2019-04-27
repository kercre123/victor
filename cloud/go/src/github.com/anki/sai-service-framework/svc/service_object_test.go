package svc

import (
	"context"
	"testing"

	"github.com/jawher/mow.cli"
	"github.com/stretchr/testify/assert"
)

func TestCompositeServiceObject_CommandSpec(t *testing.T) {
	s1 := &SimpleServiceObject{Spec: "[-a] [-b]"}
	s2 := &SimpleServiceObject{Spec: "-c -d"}
	so := NewServiceObject("test",
		WithComponent(s1),
		WithComponent(s2))
	assert.Equal(t, 2, len(so.components))
	assert.NotNil(t, so)
	assert.Equal(t, "[-a] [-b] -c -d", so.CommandSpec())
}

func TestCompositeServiceObject_Callables(t *testing.T) {
	var startCount, stopCount, killCount, initCount int
	so := &CompositeServiceObject{}
	componentCount := 4
	for i := 0; i < componentCount; i++ {
		so.Add(&SimpleServiceObject{
			CommandInitializer: func(cmd *cli.Cmd) {
				initCount++
			},
			OnStart: func(ctx context.Context) error {
				startCount++
				return nil
			},
			OnStop: func() error {
				stopCount++
				return nil
			},
			OnKill: func() error {
				killCount++
				return nil
			},
		})
	}
	assert.Equal(t, componentCount, len(so.components))
	so.CommandInitialize(&cli.Cmd{})
	assert.Equal(t, componentCount, initCount)
	assert.NoError(t, so.Start(context.Background()))
	assert.Equal(t, componentCount, startCount)
	so.Stop()
	assert.Equal(t, componentCount, stopCount)
	so.Kill()
	assert.Equal(t, componentCount, killCount)
}

func TestSimpleServiceObject_Callables(t *testing.T) {
	var startCount, stopCount, killCount, initCount int
	so := &SimpleServiceObject{
		Spec: "--opt1 [--opt2]",
		CommandInitializer: func(cmd *cli.Cmd) {
			initCount++
		},
		OnStart: func(ctx context.Context) error {
			startCount++
			return nil
		},
		OnStop: func() error {
			stopCount++
			return nil
		},
		OnKill: func() error {
			killCount++
			return nil
		},
	}
	so.CommandInitialize(&cli.Cmd{})
	assert.Equal(t, 1, initCount)
	assert.NoError(t, so.Start(context.Background()))
	assert.Equal(t, 1, startCount)
	so.Stop()
	assert.Equal(t, 1, stopCount)
	so.Kill()
	assert.Equal(t, 1, killCount)
	assert.Equal(t, "--opt1 [--opt2]", so.CommandSpec())

	// Test that there's no errors or panics calling an uninitialized
	// SimpleServiceObject
	so = &SimpleServiceObject{}
	assert.NotPanics(t, func() { so.Start(context.Background()) })
	assert.NotPanics(t, func() { so.Stop() })
	assert.NotPanics(t, func() { so.Kill() })
	assert.NotPanics(t, func() { so.CommandSpec() })
	assert.NotPanics(t, func() { so.CommandInitialize(nil) })
}
