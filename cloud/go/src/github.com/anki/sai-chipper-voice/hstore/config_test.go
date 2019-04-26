package hstore

import (
	"github.com/anki/sai-service-framework/svc"
	"github.com/stretchr/testify/assert"
	"testing"
	"time"
)

var nSize int = 10

func TestConfigDefaults(t *testing.T) {
	cfg := &Config{}
	cfg.defaults()
	var d svc.Duration
	d.Set(defaultQueueFlushInterval)
	assert.Equal(t, defaultMaxRequests, *cfg.MaxRequests)
	assert.Equal(t, defaultRecordQueueSize, *cfg.RecordQueueSize)
	assert.Equal(t, d.Duration(), cfg.BufferFlushTime.Duration())
	assert.Equal(t, defaultBufferSize, *cfg.BufferSize)
}

func TestConfigSetQueueSize(t *testing.T) {
	cfg := &Config{
		RecordQueueSize: &nSize,
	}
	cfg.defaults()
	assert.Equal(t, nSize, *cfg.RecordQueueSize)
}

func TestConfigSetMaxRequest(t *testing.T) {
	cfg := &Config{
		MaxRequests: &nSize,
	}
	cfg.defaults()
	assert.Equal(t, nSize, *cfg.MaxRequests)
}
func TestConfigSetAggregatorSize(t *testing.T) {
	cfg := &Config{
		BufferSize: &nSize,
	}
	cfg.defaults()
	assert.Equal(t, nSize, *cfg.BufferSize)
}
func TestConfigSetQueueFlushTime(t *testing.T) {
	var d svc.Duration
	d.Set("10s")
	cfg := &Config{
		BufferFlushTime: &d,
	}
	cfg.defaults()
	assert.Equal(t, d, *cfg.BufferFlushTime)
	assert.Equal(t, 10*time.Second, cfg.BufferFlushTime.Duration())
}
