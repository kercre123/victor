package certstore

import (
	"github.com/anki/sai-service-framework/svc"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/jawher/mow.cli"
)

type RobotCertificateStoreConfig struct {
	// enabled turns saving robot SDK certificates
	// to S3 on or off.
	enabled *bool

	// bucket configures the S3 bucket to store the
	// robot SDK certificates uploaded by AssociatePrimaryUser.
	bucket *string

	// prefix sets an optional path prefix to use for
	// storing robot SDK certificates in S3.
	prefix *string
}

func (cfg *RobotCertificateStoreConfig) CommandSpec() string {
	return "[--robot-cert-bucket] [--robot-cert-prefix] " +
		"[--robot-cert-store-enabled]"
}

func (cfg *RobotCertificateStoreConfig) CommandInitialize(cmd *cli.Cmd) {
	cfg.enabled = svc.BoolOpt(cmd, "robot-cert-store-enabled", false,
		"Enable storing robot self-signed SDK certificates to S3")
	cfg.bucket = svc.StringOpt(cmd, "robot-cert-bucket", "",
		"S3 Bucket to store robot self-signed certificates for SDK usage")
	cfg.prefix = svc.StringOpt(cmd, "robot-cert-prefix", "",
		"S3 Prefix to store robot self-signed certificates for SDK usage")
}

func (cfg *RobotCertificateStoreConfig) NewStore(awscfg *aws.Config) (RobotCertificateStore, error) {
	if !*cfg.enabled {
		return &DummyRobotCertificateStore{}, nil
	}
	return NewS3RobotCertificateStore(*cfg.bucket, *cfg.prefix, awscfg)
}
