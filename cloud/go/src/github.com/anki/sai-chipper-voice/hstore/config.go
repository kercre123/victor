package hstore

import (
	"github.com/anki/sai-service-framework/svc"
)

const (
	defaultRecordQueueSize    = 500
	defaultQueueFlushInterval = "1000ms"
	defaultBufferSize         = 1000
	defaultMaxRequests        = 20
	defaultMaxRetry           = 5
)

type Config struct {
	// StreamName is the Kinesis Firehose stream
	StreamName *string

	// Region is the AWS Region of the stream
	Region *string

	// RoleArn is the role to assume in order to access the Firehose
	RoleArn *string

	// RecordQueueSize is the size of channel to save records going into the buffer
	RecordQueueSize *int

	// BufferFlushTime is the wait time between periodic flushing of the records buffer
	BufferFlushTime *svc.Duration

	// BufferSize is the size of the records buffer before flushing
	BufferSize *int

	// MaxRequests is the number of firehose requests to send concurrently
	MaxRequests *int

	// MaxRetries is the number of time to retry a failed firehose put operation
	MaxRetry *int
}

func (cfg *Config) defaults() {
	if cfg.RecordQueueSize == nil {
		size := defaultRecordQueueSize
		cfg.RecordQueueSize = &size
	} else if *cfg.RecordQueueSize == 0 {
		*cfg.RecordQueueSize = defaultRecordQueueSize
	}

	if cfg.MaxRetry == nil {
		retry := defaultMaxRetry
		cfg.MaxRetry = &retry
	} else if *cfg.MaxRetry == 0 {
		*cfg.MaxRetry = defaultMaxRetry
	}

	if cfg.BufferFlushTime == nil {
		var d svc.Duration
		d.Set(defaultQueueFlushInterval)
		cfg.BufferFlushTime = &d
	} else if cfg.BufferFlushTime.Duration() == 0 {
		cfg.BufferFlushTime.Set(defaultQueueFlushInterval)
	}

	if cfg.BufferSize == nil {
		size := defaultBufferSize
		cfg.BufferSize = &size
	} else if *cfg.BufferSize == 0 {
		*cfg.BufferSize = defaultBufferSize
	}

	if cfg.MaxRequests == nil {
		maxR := defaultMaxRequests
		cfg.MaxRequests = &maxR
	} else if *cfg.MaxRequests == 0 {
		*cfg.MaxRequests = defaultMaxRequests
	}
}
