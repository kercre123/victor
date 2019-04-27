// Copyright 2016 Anki, Inc.

package logsender

import (
	"errors"
	"testing"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/kinesis"
)

var checkTests = []struct {
	mode        string
	errExpected bool
}{
	{"putrecords_error", true},
	{"norecords", true},
	{"record_error", true},
	{"ok", false},
}

func TestCheckConnectivity(t *testing.T) {
	const maxCheckConnectivityRetries = 3

	// putrecords errror
	// insufficnet records
	// single record error
	// no error
	var callCount int
	var mode string

	put := func(input *kinesis.PutRecordsInput) (*kinesis.PutRecordsOutput, error) {
		callCount++
		switch mode {
		case "putrecords_error":
			return nil, errors.New("PutRecords failed")

		case "norecords":
			return &kinesis.PutRecordsOutput{
				FailedRecordCount: aws.Int64(0),
			}, nil

		case "record_error":
			return &kinesis.PutRecordsOutput{
				FailedRecordCount: aws.Int64(1),
				Records: []*kinesis.PutRecordsResultEntry{
					{
						ErrorCode:    aws.String("oops"),
						ErrorMessage: aws.String("is broke"),
					},
				},
			}, nil

		case "ok":
			return &kinesis.PutRecordsOutput{
				FailedRecordCount: aws.Int64(0),
				Records: []*kinesis.PutRecordsResultEntry{
					{
						SequenceNumber: aws.String("seqnum"),
					},
				},
			}, nil
		default:
			panic("unknown mode: " + mode)
		}
	}

	k := &fakeKinesis{customResponder: put}
	for _, test := range checkTests {
		callCount = 0
		mode = test.mode
		err := checkConnectivity("stream", k)
		if test.errExpected {
			if err == nil {
				t.Errorf("mode=%s no error returned", mode)
			} else if callCount != 1 {
				t.Errorf("mode=%s incorrect retry count expected=%d actual=%d",
					mode, 1, callCount)
			}
		} else {
			if err != nil {
				t.Errorf("mode=%s unexpected error=%s", mode, err)
			}
		}
	}
}
