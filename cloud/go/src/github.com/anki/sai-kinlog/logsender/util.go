// Copyright 2016 Anki, Inc.

package logsender

import (
	"fmt"

	"github.com/anki/sai-kinlog/logsender/logrecord"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/kinesis"
)

// checkConnectivity sends a null event record to Kinesis to validate that
// the configuration and connectivity is valid.
//
// The null record consist of a normal compressed entry set, but holds
// only a metadata header with no actual entries following.
func checkConnectivity(stream string, k KinesisPutter) (err error) {
	r := new(logrecord.Encoder)
	r.Reset(nullMetadata)
	req := &kinesis.PutRecordsInput{
		StreamName: aws.String(stream),
		Records: []*kinesis.PutRecordsRequestEntry{
			{
				Data:         r.Data(),
				PartitionKey: aws.String("__null__"),
			},
		},
	}

	var resp *kinesis.PutRecordsOutput
	resp, err = k.PutRecords(req)
	if err != nil {
		return fmt.Errorf("Kinesis check failed: %v", err)
	}
	if len(resp.Records) != 1 {
		return fmt.Errorf("Kinesis check failed: No records returned")
	}
	if resp.Records[0].ErrorCode != nil {
		return fmt.Errorf("Kinesis check failed: %v", aws.StringValue(resp.Records[0].ErrorMessage))
	}
	return nil
}
