package svc

import (
	"testing"

	"github.com/jawher/mow.cli"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestServerTlsConfig_Load_Errors(t *testing.T) {
	cfg := &ServerTLSConfig{}
	tlscfg, err := cfg.Load()
	assert.Nil(t, tlscfg)
	assert.NoError(t, err)

	cfg.Enabled = boolPtr(true)
	tlscfg, err = cfg.Load()
	assert.Nil(t, tlscfg)
	assert.Error(t, err)
}

func TestServerTlsConfig_Load_Simple(t *testing.T) {
	cmd := cli.App("test", "Test stuff")

	cfg := NewServerTLSConfig("test")
	cfg.CommandInitialize(cmd.Cmd)

	cfg.Enabled = boolPtr(true)
	cfg.validateClient = boolPtr(false)
	cfg.tlsCert.certFile = ptr("testdata/certificate.pem")
	cfg.tlsCert.keyFile = ptr("testdata/key.pem")

	tlscfg, err := cfg.Load()
	require.NoError(t, err)
	assert.NotNil(t, tlscfg)
}
