package dockerutil

import (
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/dynamodb"
)

// DynamoSuite is a base test suite type which launches a local Amazon
// DynamoDB-compatible docker container and configures the Amazon AWS
// SDK's dynamo client to point to it.
//
// Unlike many other local implementations of an AWS backend, the
// local Dynamo implementation is provided by Amazon. See
// https://docs.aws.amazon.com/amazondynamodb/latest/developerguide/DynamoDBLocal.html
//
// This will also be home to some helper functions at some point.
type DynamoSuite struct {
	Suite

	// Dynamo is the AWS SDK client, which will be assigned when
	// SetupSuite() completes successfully.
	Dynamo *dynamodb.DynamoDB
	Config *aws.Config
}

func (s *DynamoSuite) SetupSuite() {
	s.Start = StartDynamoContainer
	s.Suite.SetupSuite()

	s.Config = &aws.Config{
		Region:      aws.String("us-east-1"),
		Endpoint:    aws.String("http://" + s.Container.Addr()),
		DisableSSL:  aws.Bool(true),
		Credentials: credentials.NewStaticCredentials("ignored", "ignored", ""),
	}

	s.Dynamo = dynamodb.New(session.New(), s.Config)
}

func (s *DynamoSuite) ListTableNames() []string {
	names := []string{}
	if rsp, err := s.Dynamo.ListTables(&dynamodb.ListTablesInput{}); err == nil {
		for _, t := range rsp.TableNames {
			names = append(names, *t)
		}
	}
	return names
}

// DeleteAllTables will, exactly as the name implies, delete *EVERY*
// table in DynamoDB. Needless to say, you should never, ever, EVER
// run this against an actual connection to live AWS.
func (s *DynamoSuite) DeleteAllTables() {
	tables := s.ListTableNames()
	for _, t := range tables {
		_, err := s.Dynamo.DeleteTable(&dynamodb.DeleteTableInput{
			TableName: aws.String(t),
		})
		if err != nil {
			s.T().Fatalf("Unable to delete dynamo table %s: %s", t, err)
		}
	}
}
