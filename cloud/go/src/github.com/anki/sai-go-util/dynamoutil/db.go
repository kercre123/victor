package dynamoutil

import (
	"errors"
	"fmt"
	"os"
	"reflect"
	"strings"
	"time"

	"github.com/anki/sai-go-util/awsutil"
	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/metricsutil"
	"github.com/anki/sai-go-util/strutil"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/dynamodb"
	"github.com/aws/aws-sdk-go/service/dynamodb/dynamodbiface"
	"github.com/cenkalti/backoff"
)

// ----------------------------------------------------------------------
// Dynamo Configuration & Initialization
// ----------------------------------------------------------------------

var (
	DynamoDB         dynamodbiface.DynamoDBAPI
	TablePrefix      string
	DynamoDbEndpoint string

	ErrorPrefixNotSet  = errors.New("Table prefix not configured")
	ErrorUninitialized = errors.New("DynamoD client not initialized")
)

func init() {
	envconfig.String(&DynamoDbEndpoint, "DYNAMODB_URL", "dynamo", "URL of Dynamo server (overides aws-region)")
	envconfig.String(&TablePrefix, "DYNAMO_PREFIX", "table-prefix", "Table prefix name")
}

// OpenDynamo configures and opens a session to Dynamo, optionally
// validating that the registered tables have been created as well.
func OpenDynamo(checkTables bool) error {
	if TablePrefix == "" {
		return ErrorPrefixNotSet
	}

	cfg, err := awsutil.GetConfig()
	if err != nil {
		return err
	}

	if DynamoDbEndpoint != "" {
		cfg.Endpoint = aws.String(DynamoDbEndpoint)
		if strings.HasPrefix(DynamoDbEndpoint, "http:") {
			cfg.DisableSSL = aws.Bool(true)
		}
	}

	dynamo := dynamodb.New(session.New(), cfg)
	metricsutil.AddAWSMetricsHandlers(dynamo.Client)
	DynamoDB = dynamo

	if checkTables {
		err := CheckTables()
		if err != nil {
			return err
		}
	}

	return nil
}

// CheckTables checks that all the registered tables exist in Dynamo.
func CheckTables() error {
	for _, table := range Tables {
		if _, err := DynamoDB.DescribeTable(table.NewDescribeTableInput()); err != nil {
			return fmt.Errorf("Failed to query Dynamo table %q: %s", table.Name, err)
		}
	}
	return nil
}

// CreateTables ensures that all the registered tables have been
// created in Dynamo.
func CreateTables() error {
	for _, t := range Tables {
		// Check to see if the table already exists first
		if _, err := DynamoDB.DescribeTable(t.NewDescribeTableInput()); err != nil {
			fmt.Println("Creating table", t)
			if _, err := DynamoDB.CreateTable(t.NewCreateTableInput()); err != nil {
				fmt.Fprintf(os.Stderr, "Create table failed. %s\n", err)
				return err
			}
		}
	}
	return nil
}

// DeleteTables deletes all registered tables - needless to say, BE
// CAREFUL DOING THIS.
func DeleteTables() error {
	for _, t := range Tables {
		if _, err := DynamoDB.DeleteTable(t.NewDeleteTableInput()); err != nil {
			fmt.Fprintf(os.Stderr, "Delete table failed. %s\n", err)
			return err
		}
	}
	return nil
}

// BatchWriteItems executes a batch write request, handling partial
// success by repeating the request for unsuccesful items using a backoff
// mechanism until the request completes fully.
func BatchWriteItems(req *dynamodb.BatchWriteItemInput) (err error) {
	berr := backoff.Retry(func() error {
		resp, berr := DynamoDB.BatchWriteItem(req)
		if berr != nil {
			err = berr // hard error
			return nil
		}
		if len(resp.UnprocessedItems) == 0 {
			return nil
		}
		req.RequestItems = resp.UnprocessedItems
		return errors.New("Unprocessed items")
	}, newDefaultBackoff())
	if err != nil {
		return err
	}
	return berr
}

// CheckStatus checks the connection to dynamo by verifying that the
// tables are valid.
func CheckStatus() error {
	return CheckTables()
}

var (
	// Used for id offsets; do not change.
	idEpoch = time.Date(2014, time.January, 1, 0, 0, 0, 0, time.UTC)
)

// GenerateID allocates a 21 byte random id, to use as a database
// primary key. The Id uses characters from the set
// "23456789abcdefghjkmnpqrstuvwxyzABCDEFGHJKMNPQRSTUVWXYZ"
//
// The first 4 bytes encode the number of hours since idEpoch
// which is meant to help bucketing of indexes in the DB.
//
// The remaining bytes are random.
//
// Adapted from sai-go-accounts, should be moved to sai-go-util (along
// with the rest of this package).
func GenerateID() (string, error) {
	// Add a prefix to the id representing the number of hours since the epoch
	// 50 years == 19 bits.
	delta := uint64(time.Since(idEpoch).Hours())
	timePrefix := strutil.EncodeInt(strutil.AllSafe, 4, delta) // 4 bytes

	// 19 bytes from the AllChar set gets us ~108 bits of randomness
	uuid, err := strutil.Str(strutil.AllSafe, 19)
	if err != nil {
		return "", err
	}

	return timePrefix + uuid, nil
}

// AssignID ensures that an object has a unique value in its Id field,
// generating one with GenerateID() if necessary.
func AssignID(obj interface{}) error {
	entry := reflect.ValueOf(obj).Elem()
	if f := entry.FieldByName("Id"); f.IsValid() {
		if f.String() == "" {
			id, err := GenerateID()
			if err != nil {
				return err
			}
			f.SetString(id)
		}
		return nil
	}
	panic("AssignId expects a struct with an Id field!")
}

// defaultBackoff generates a reasonable backoff strategy for dynamo requests
func newDefaultBackoff() backoff.BackOff {
	b := &backoff.ExponentialBackOff{
		InitialInterval:     50 * time.Millisecond,
		RandomizationFactor: backoff.DefaultRandomizationFactor,
		Multiplier:          1.25,
		MaxInterval:         1 * time.Second,
		MaxElapsedTime:      10 * time.Second,
		Clock:               backoff.SystemClock,
	}
	b.Reset()
	return b
}
