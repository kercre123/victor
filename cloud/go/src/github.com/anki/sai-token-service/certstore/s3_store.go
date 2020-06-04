package certstore

import (
	"bytes"
	"context"
	"crypto/x509"
	"encoding/pem"
	"errors"
	"fmt"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/metricsutil"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

const (
	contentType = "application/octet-stream"
)

var (
	errorBucketNotSet = errors.New("Robot Session Certificate Store bucket not set")
	errorInvalidPEM   = errors.New("Invalid PEM data")
)

// S3RobotCertificateStore stores robot session certificates in 3S.
type S3RobotCertificateStore struct {
	bucket string
	prefix string
	s3svc  *s3.S3
}

func NewS3RobotCertificateStore(bucket, prefix string, cfg *aws.Config) (RobotCertificateStore, error) {
	if bucket == "" {
		return nil, errorBucketNotSet
	}

	s3svc := s3.New(session.Must(session.NewSession(cfg)))
	metricsutil.AddAWSMetricsHandlers(s3svc.Client)

	return &S3RobotCertificateStore{
		bucket: bucket,
		prefix: prefix,
		s3svc:  s3svc,
	}, nil
}

func (s *S3RobotCertificateStore) Store(ctx context.Context, product, robotID string, cert []byte) error {
	if err := validate(cert); err != nil {
		alog.Warn{
			"action": "store_certificate",
			"status": "invalid_certificate",
			"error":  err,
		}.LogC(ctx)
		return status.Errorf(codes.InvalidArgument, "%s", err)
	}
	path := s.makePath(product, robotID)
	req := &s3.PutObjectInput{
		ContentType:  aws.String(contentType),
		CacheControl: aws.String("no-cache,no-store,max-age=0,must-revalidate"),
		Bucket:       aws.String(s.bucket),
		Key:          aws.String(path),
		Body:         aws.ReadSeekCloser(bytes.NewReader(cert)),
	}
	_, err := s.s3svc.PutObject(req)
	if err == nil {
		alog.Info{
			"action": "store_certificate",
			"status": alog.StatusOK,
			"path":   path,
		}.LogC(ctx)
	}
	return err
}

func (s *S3RobotCertificateStore) makePath(product, robotID string) string {
	return fmt.Sprintf("%s/%s/%s", s.prefix, product, robotID)
}

func validate(cert []byte) error {
	block, _ := pem.Decode(cert)
	if block == nil {
		return errorInvalidPEM
	}

	if _, err := x509.ParseCertificate(block.Bytes); err != nil {
		return err
	}
	return nil
}
