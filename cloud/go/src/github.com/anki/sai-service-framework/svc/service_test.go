package svc

import (
	"context"
	"testing"

	"github.com/jawher/mow.cli"
	"github.com/stretchr/testify/assert"
)

func TestService_Context(t *testing.T) {
	s := &Service{}
	ctx := WithService(context.Background(), s)
	assert.Equal(t, s, GetService(ctx))
	assert.Nil(t, GetService(context.Background()))
}

func TestService_EmptyService(t *testing.T) {
	s := &Service{}
	assert.NotPanics(t, func() { s.CommandInitialize(&cli.Cmd{}) })
	assert.Equal(t, "", s.CommandSpec())
	assert.Equal(t, errorServiceObjectNotSet, s.Start(context.Background()))
	assert.Equal(t, errorServiceObjectNotSet, s.Stop())
}

func TestService_GlobalCallables(t *testing.T) {
	var startCount, stopCount, initCount int
	var cfg []ConfigFn
	componentCount := 4

	for i := 0; i < componentCount; i++ {
		cfg = append(cfg, WithComponent(&SimpleServiceObject{
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
		}))
	}

	so := NewServiceObject("test", cfg...)
	assert.NotNil(t, so)
	s, err := NewService("test", "test", WithGlobal(so))
	assert.NoError(t, err)
	assert.Equal(t, so, s.Global)
	assert.Equal(t, "test", s.Name)
	assert.NotNil(t, s.Metrics)
	assert.NotNil(t, s.Cli)
	assert.Equal(t, componentCount, initCount) // CommandInitialize is called by NewService()
	assert.NoError(t, s.Start(context.Background()))
	assert.Equal(t, componentCount, startCount)
	assert.NotPanics(t, func() { s.Stop() })
	assert.Equal(t, componentCount, stopCount)
}
