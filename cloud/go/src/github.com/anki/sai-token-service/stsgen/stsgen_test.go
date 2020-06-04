package stsgen

import (
	"context"
	"testing"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/sts"
	"github.com/aws/aws-sdk-go/service/sts/stsiface"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

type mockSTS struct {
	stsiface.STSAPI
	getFederationTokenFunc func(input *sts.GetFederationTokenInput) (*sts.GetFederationTokenOutput, error)
}

func (m *mockSTS) GetFederationToken(input *sts.GetFederationTokenInput) (*sts.GetFederationTokenOutput, error) {
	return m.getFederationTokenFunc(input)
}

func TestNewTokenPolicy(t *testing.T) {
	require := require.New(t)
	assert := assert.New(t)
	var param *sts.GetFederationTokenInput
	ms := &mockSTS{
		getFederationTokenFunc: func(input *sts.GetFederationTokenInput) (*sts.GetFederationTokenOutput, error) {
			param = input
			return &sts.GetFederationTokenOutput{
				Credentials: &sts.Credentials{
					AccessKeyId: aws.String("test-access-key"),
				},
			}, nil
		},
	}

	gen := &StsGenerator{
		tokenDuration: 123 * time.Second,
		logUploadARN:  "s3://my-bucket/prefix/%USER_ID%/%THING_ID%/*",
		stssvc:        ms,
	}

	creds, err := gen.NewToken(context.Background(), "user-id", "req-id")
	require.Nil(err)

	assert.EqualValues(123, aws.Int64Value(param.DurationSeconds))
	assert.Equal("uid=user-id", aws.StringValue(param.Name))
	expectedPolicy := `{"Version":"2012-10-17","Statement":[{"Effect":"Allow","Action":["s3:PutObject"],"Resource":["s3://my-bucket/prefix/user-id/cmVxLWlk/*"]}]}`
	assert.Equal(expectedPolicy, aws.StringValue(param.Policy))
	assert.Equal("test-access-key", aws.StringValue(creds.AccessKeyId))
}
