package stsgen

import (
	"context"
	"errors"
	"fmt"
	"time"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/metricsutil"
	"github.com/anki/sai-service-framework/svc"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/sts"
	cli "github.com/jawher/mow.cli"
)

const (
	defaultStsTokenDuration = 1 * time.Hour
)

// StsGeneratorConfig collects configuration information for an StsGenerator.
type StsGeneratorConfig struct {
	enabled          *bool
	tokenDuration    *svc.Duration
	iamUserAccessKey *string
	iamUserSecretKey *string
	logUploadARN     *string
}

func (cfg *StsGeneratorConfig) CommandSpec() string {
	return "[--sts-enabled] [--sts-token-duration] [--sts-iam-user-access-key --sts-iam-user-secret-key --sts-s3-log-arn]"
}

func (cfg *StsGeneratorConfig) CommandInitialize(cmd *cli.Cmd) {
	cfg.enabled = svc.BoolOpt(cmd, "sts-enabled", true,
		"Enabled generation of STS tokens")
	cfg.tokenDuration = svc.DurationOpt(cmd, "sts-token-duration", defaultStsTokenDuration,
		"Duration issued STS tokens will be valid for")
	cfg.iamUserAccessKey = svc.StringOpt(cmd, "sts-iam-user-access-key", "",
		"IAM access key for STS federation user")
	cfg.iamUserSecretKey = svc.StringOpt(cmd, "sts-iam-user-secret-key", "",
		"IAM secret access key for STS federation user")
	cfg.logUploadARN = svc.StringOpt(cmd, "sts-s3-log-arn", "",
		"S3 ARN Prefix for STS policy filter (eg. arn:aws:s3:::bucket-name/prefix/%USER_ID%/%THING_ID%/*")
}

func (cfg *StsGeneratorConfig) NewSTSGenerator(awscfg *aws.Config) (*StsGenerator, error) {
	if !*cfg.enabled {
		alog.Warn{"action": "new_sts_generator", "status": "disabled"}.Log()
		return nil, nil
	}
	if *cfg.iamUserAccessKey == "" || *cfg.iamUserSecretKey == "" || *cfg.logUploadARN == "" {
		return nil, errors.New("Missing STS parameters")
	}
	creds := credentials.NewStaticCredentials(*cfg.iamUserAccessKey, *cfg.iamUserSecretKey, "")
	svc := sts.New(session.New(), awscfg.WithCredentials(creds))
	metricsutil.AddAWSMetricsHandlers(svc.Client)
	gen := &StsGenerator{
		tokenDuration:    cfg.tokenDuration.Duration(),
		iamUserAccessKey: *cfg.iamUserAccessKey,
		iamUserSecretKey: *cfg.iamUserSecretKey,
		logUploadARN:     *cfg.logUploadARN,
		stssvc:           svc,
	}
	if _, err := gen.NewToken(context.Background(), "tms-startup", "tms-startup"); err != nil {
		return nil, fmt.Errorf("Invalid STS configuration: %v", err)
	}
	alog.Info{"action": "new_sts_generator", "status": "enabled", "log_upload_arn": *cfg.logUploadARN}.Log()
	return gen, nil
}
