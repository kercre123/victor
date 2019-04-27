// Copyright 2016 Anki, Inc.

package kinreader

import (
	"fmt"
	"sync"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/dynamodb"
)

var (
	// DefaultDynamoMaxTries specifies the maximum number of attempts a
	// DynamoDBCheckpointer will make to read or commit data to a DymamoDB table.
	DefaultDynamoMaxTries = 100

	// DefaultDynamoCapacity specifies the default read/write capacity of
	// newly created DynamoDB tables.
	DefaultDynamoCapacity int64 = 10
)

// DynamoDB defines the interface used to talk to a DynamoDB table by a
// DynamoCheckpointer.
type DynamoDB interface {
	CreateTable(input *dynamodb.CreateTableInput) (*dynamodb.CreateTableOutput, error)
	DescribeTable(input *dynamodb.DescribeTableInput) (*dynamodb.DescribeTableOutput, error)
	PutItem(input *dynamodb.PutItemInput) (*dynamodb.PutItemOutput, error)
	Scan(input *dynamodb.ScanInput) (*dynamodb.ScanOutput, error)
	WaitUntilTableExists(input *dynamodb.DescribeTableInput) error
}

// DynamoDBCheckpointer stores and retrieves Checkpoints from a DynamoDB table.
// The table must have a provisioned read/write capacity to handle the number
// of checkpoint requests coming from ShardProcessors.
type DynamoDBCheckpointer struct {
	dyn       DynamoDB
	tableName string
	m         sync.Mutex
}

// NewDynamoDBCheckpointer returns a DynamoDBCheckpointer configured to log
// checkpoints to the specified DynamoDB table.  It will attempt to create the
// table using the DefaultDynamoCapacity capacity if it does not already exist.
//
// If dyn is nil then a DynamoDB instance will be created using system default
// configuration.
func NewDynamoDBCheckpointer(tableName string, dyn DynamoDB) (*DynamoDBCheckpointer, error) {
	dcp := &DynamoDBCheckpointer{
		tableName: tableName,
	}
	if dyn == nil {
		cfg := &aws.Config{
			MaxRetries: aws.Int(DefaultDynamoMaxTries),
		}
		dcp.dyn = dynamodb.New(session.New(cfg))
	} else {
		dcp.dyn = dyn
	}

	if err := createDynCpTable(dcp.dyn, tableName); err != nil {
		return nil, fmt.Errorf("Failed to create DynamoDB table: %v", err)
	}
	return dcp, nil
}

func createDynCpTable(dyn DynamoDB, tableName string) error {
	_, err := dyn.DescribeTable(&dynamodb.DescribeTableInput{
		TableName: aws.String(tableName),
	})
	if err != nil {
		if err, ok := err.(awserr.Error); ok {
			if err.Code() != "ResourceNotFoundException" {
				return err
			}
		} else {
			return err
		}
	} else {
		return nil
	}
	req := &dynamodb.CreateTableInput{
		AttributeDefinitions: []*dynamodb.AttributeDefinition{
			{
				AttributeName: aws.String("ShardId"),
				AttributeType: aws.String("S"),
			},
		},
		KeySchema: []*dynamodb.KeySchemaElement{
			{
				AttributeName: aws.String("ShardId"),
				KeyType:       aws.String("HASH"),
			},
		},
		ProvisionedThroughput: &dynamodb.ProvisionedThroughput{
			ReadCapacityUnits:  aws.Int64(DefaultDynamoCapacity),
			WriteCapacityUnits: aws.Int64(DefaultDynamoCapacity),
		},
		TableName: aws.String(tableName),
	}
	if _, err := dyn.CreateTable(req); err != nil {
		return err
	}
	return dyn.WaitUntilTableExists(&dynamodb.DescribeTableInput{
		TableName: aws.String(tableName),
	})
}

// SetCheckpoint stores a checkpoint in a DynamoDB table.
func (dcp *DynamoDBCheckpointer) SetCheckpoint(cp Checkpoint) error {
	dcp.m.Lock()
	defer dcp.m.Unlock()

	t, _ := cp.Time.MarshalText()
	item := &dynamodb.PutItemInput{
		TableName: aws.String(dcp.tableName),
		Item: map[string]*dynamodb.AttributeValue{
			"ShardId": {
				S: aws.String(cp.ShardId),
			},
			"Time": {
				S: aws.String(string(t)),
			},
			"Checkpoint": {
				S: aws.String(cp.Checkpoint),
			},
		},
	}

	_, err := dcp.dyn.PutItem(item)
	if err != nil {
		return err
	}
	return nil
}

// AllCheckpoints retrieves all known checkpoints from a DynamoDB table.
func (dcp *DynamoDBCheckpointer) AllCheckpoints() (cps Checkpoints, err error) {
	dcp.m.Lock()
	defer dcp.m.Unlock()

	cps = make(Checkpoints)
	q := &dynamodb.ScanInput{
		TableName:      aws.String(dcp.tableName),
		ConsistentRead: aws.Bool(true),
	}
	for {
		resp, err := dcp.dyn.Scan(q)
		if err != nil {
			return nil, err
		}
		for _, item := range resp.Items {
			var t time.Time
			t.UnmarshalText([]byte(dynStringValue(item["Time"])))
			rec := Checkpoint{
				ShardId:    dynStringValue(item["ShardId"]),
				Checkpoint: dynStringValue(item["Checkpoint"]),
				Time:       t,
			}
			if rec.ShardId != "" && rec.Checkpoint != "" {
				cps[rec.ShardId] = &rec
			}
		}
		if lk := resp.LastEvaluatedKey; len(lk) == 0 {
			break
		} else {
			q.ExclusiveStartKey = lk
		}
	}
	return cps, nil
}

func dynStringValue(v *dynamodb.AttributeValue) string {
	if v == nil {
		return ""
	}
	return aws.StringValue(v.S)
}
