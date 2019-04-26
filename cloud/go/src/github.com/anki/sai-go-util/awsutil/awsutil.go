package awsutil

import (
	"errors"

	"github.com/anki/sai-go-util/envconfig"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials"
)

var (
	AwsRegionName string = "us-west-2"
	AwsAccessKey  string
	AwsSecretKey  string

	ErrorRegionNotSet = errors.New("AWS region not configured")
)

func init() {
	envconfig.String(&AwsRegionName, "AWS_REGION", "aws-region", "AWS region to connect to")
	envconfig.String(&AwsAccessKey, "AWS_ACCESS_KEY", "", "AWS access key")
	envconfig.String(&AwsSecretKey, "AWS_SECRET_KEY", "", "AWS secret key") // TODO: AWS SDK uses AWS_ACCESS_SECRET_KEY
}

func GetConfig() (*aws.Config, error) {
	if AwsRegionName == "" {
		return nil, ErrorRegionNotSet
	}

	cfg := &aws.Config{
		Region: aws.String(AwsRegionName),
	}

	if AwsAccessKey != "" && AwsSecretKey != "" {
		cfg = cfg.WithCredentials(credentials.NewStaticCredentials(
			AwsAccessKey, AwsSecretKey, ""))
	}

	return cfg, nil
}
