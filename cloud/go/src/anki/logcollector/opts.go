package logcollector

import (
	"anki/token"
	"net/http"
)

type options struct {
	server     bool
	tokener    token.Accessor
	httpClient *http.Client

	bucketName   string
	s3BasePrefix string
	awsRegion    string
}

// Option defines an option that can be set on the server
type Option func(o *options)

// WithServer specifies that an IPC server should be started so other processes
// can use log collection
func WithServer() Option {
	return func(o *options) {
		o.server = true
	}
}

// WithHTTPClient specifies the HTTP client to use
func WithHTTPClient(httpClient *http.Client) Option {
	return func(o *options) {
		o.httpClient = httpClient
	}
}

// WithTokener specifies that the given token.Accessor should be used to obtain
// authorization credentials (used to retrieve USerID)
func WithTokener(value token.Accessor) Option {
	return func(o *options) {
		o.tokener = value
	}
}

// WithBucketName specifies the S3 bucket name in the cloud
func WithBucketName(bucketName string) Option {
	return func(o *options) {
		o.bucketName = bucketName
	}
}

// WithS3BasePrefix specifies the S3 key prefix in the cloud
func WithS3BasePrefix(s3BasePrefix string) Option {
	return func(o *options) {
		o.s3BasePrefix = s3BasePrefix
	}
}

// WithAwsRegion specifies the AWS region
func WithAwsRegion(awsRegion string) Option {
	return func(o *options) {
		o.awsRegion = awsRegion
	}
}
