package logcollector

import (
	"anki/token"
	"net/http"
	"net/url"
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

// WithS3UrlPrefix specifies the S3 bucket and key prefix in the cloud
// E.g. s3://anki-device-logs-dev/victor
func WithS3UrlPrefix(s3UrlPrefix string) Option {
	return func(o *options) {
		parsedURL, err := url.Parse(s3UrlPrefix)
		if err == nil {
			o.bucketName = parsedURL.Host
			o.s3BasePrefix = parsedURL.Path
		}
	}
}

// WithAwsRegion specifies the AWS region
func WithAwsRegion(awsRegion string) Option {
	return func(o *options) {
		o.awsRegion = awsRegion
	}
}
