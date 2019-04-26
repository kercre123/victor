package dynamotest

import (
	"fmt"

	"github.com/anki/sai-go-util/awsutil"
	"github.com/anki/sai-go-util/dockerutil"
	"github.com/anki/sai-go-util/dynamoutil"
)

type Suite struct {
	dockerutil.Suite
}

func (s *Suite) SetupSuite() {
	s.Suite.Start = dockerutil.StartDynamoContainer
	s.Suite.SetupSuite()

	dynamoutil.DynamoDbEndpoint = fmt.Sprintf("http://%s", s.Container.Addr())
	awsutil.AwsRegionName = "local-dynamo"
	awsutil.AwsAccessKey = "test-key"
	awsutil.AwsSecretKey = "test-secret"
	dynamoutil.TablePrefix = "local-"

	fmt.Println("DYNAMO IP", dynamoutil.DynamoDbEndpoint)

	dynamoutil.OpenDynamo(false)
}

func (s *Suite) SetupTest() {
	dynamoutil.DeleteTables()
	dynamoutil.CreateTables()
}
