package grpcclient

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestClientConfig_Prefix(t *testing.T) {
	cfg := NewClientConfig("test")
	assert.Equal(t, "test-grpc-client-foo", cfg.argname("foo"))
	assert.NotNil(t, cfg.peerCertConfig)
	assert.Equal(t, "test-grpc-client-peer", cfg.peerCertConfig.Prefix)
}
