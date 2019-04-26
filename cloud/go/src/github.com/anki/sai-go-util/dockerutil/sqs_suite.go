package dockerutil

import (
	"github.com/aalpern/go-metrics"
	"github.com/anki/sai-go-util/metricsutil"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/sqs"
)

type SQSSuite struct {
	Suite
	SQS        *sqs.SQS
	Config     *aws.Config
	Endpoint   string
	MetricsReg *metrics.Registry
}

func (s *SQSSuite) SetupSuite() {
	s.Start = StartSQSContainer
	s.Suite.SetupSuite()
	s.Endpoint = "http://" + s.Container.Addr()
	s.Config = &aws.Config{
		Region:      aws.String("us-east-1"),
		Endpoint:    aws.String(s.Endpoint),
		DisableSSL:  aws.Bool(true),
		Credentials: credentials.NewStaticCredentials("x", "x", ""),
	}
	s.SQS = sqs.New(session.New(), s.Config)
	s.MetricsReg = metricsutil.AddAWSMetricsHandlers(s.SQS.Client)
}

func (s *SQSSuite) CreateQueue(name string) string {
	rsp, err := s.SQS.CreateQueue(&sqs.CreateQueueInput{
		QueueName: aws.String(name),
	})
	if err != nil {
		s.T().Fatalf("Failed to create SQS queue: %s", err)
	}
	return *rsp.QueueUrl
}

func (s *SQSSuite) DeleteQueue(url string) {
	_, err := s.SQS.DeleteQueue(&sqs.DeleteQueueInput{
		QueueUrl: aws.String(url),
	})
	if err != nil {
		s.T().Fatalf("Failed to delete SQS queue: %s", err)
	}
}
