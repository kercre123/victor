// Copyright 2016 Anki, Inc.

package logsender

import (
	"sync"

	"github.com/aws/aws-sdk-go/service/kinesis"
)

type kresp struct {
	ok  *kinesis.PutRecordsOutput
	err error
}
type fakeKinesis struct {
	responses       []kresp
	customResponder func(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error)
	m               sync.Mutex
	calls           []*kinesis.PutRecordsInput
}

func (k *fakeKinesis) PutRecords(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error) {
	// acquire lock for call logging as PutRecords may be called in parallel for
	// concurrent flushers.
	k.m.Lock()
	k.calls = append(k.calls, input)
	k.m.Unlock()

	if k.customResponder != nil {
		return k.customResponder(input)
	}

	if len(k.responses) == 0 {
		panic("No responses available")
	}
	next := k.responses[0]
	k.responses = k.responses[1:]
	return next.ok, next.err
}
