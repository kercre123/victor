package stsgen

import (
	"context"
	"encoding/base64"
	"fmt"
	"strings"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/sts"
	"github.com/aws/aws-sdk-go/service/sts/stsiface"
)

const (
	tokUserId  = "%USER_ID%"
	tokThingId = "%THING_ID%"
)

// Generator creates new STS tokens.
type Generator interface {
	// NewToken generates a token for a supplied user and requester (eg. robot) id.
	NewToken(ctx context.Context, userID, requesterID string) (*sts.Credentials, error)
	// Enabled returns true if token generation is enabled.
	Enabled() bool
}

// StsGenerator implements the Generator interface.
type StsGenerator struct {
	tokenDuration    time.Duration
	iamUserAccessKey string
	iamUserSecretKey string
	logUploadARN     string
	stssvc           stsiface.STSAPI
}

// Enabled returns true if s is a non-nil instance of StsGenerator.
func (s *StsGenerator) Enabled() bool {
	return s != nil
}

// NewToken generates a new STS token.
func (s *StsGenerator) NewToken(ctx context.Context, userID, requestorID string) (*sts.Credentials, error) {
	policy := s.createUserPolicy(userID, requestorID)
	jsonPolicy, err := policy.toJson()
	if err != nil {
		return nil, fmt.Errorf("policy encoded failed: %v", err)
	}

	// "name of the federated user" - Max of 32 characters
	tokenName := fmt.Sprintf("uid=%s", userID)
	if len(tokenName) > 32 {
		tokenName = tokenName[:32]
	}

	input := &sts.GetFederationTokenInput{
		DurationSeconds: aws.Int64(int64(s.tokenDuration / time.Second)),
		Name:            aws.String(tokenName),
		Policy:          aws.String(jsonPolicy),
	}

	result, err := s.stssvc.GetFederationToken(input)
	if err != nil {
		return nil, err
	}
	return result.Credentials, nil
}

func (s *StsGenerator) createUserPolicy(userId, requestorId string) policyDocument {
	return policyDocument{
		Version: "2012-10-17",
		Statement: []statementEntry{
			{
				Effect: "Allow",
				Action: []string{
					"s3:PutObject",
				},
				Resource: []string{
					createLogARNForUser(s.logUploadARN, userId, requestorId),
				},
			},
		},
	}
}

func createLogARNForUser(baseARN, userId, requestorId string) string {
	thingId := base64.StdEncoding.EncodeToString([]byte(requestorId))
	baseARN = strings.Replace(baseARN, tokUserId, userId, -1)
	baseARN = strings.Replace(baseARN, tokThingId, thingId, -1)
	return baseARN
}
