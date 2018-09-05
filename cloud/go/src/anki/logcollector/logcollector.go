package logcollector

import (
	"anki/log"
	"anki/robot"
	"anki/token"
	"context"
	"fmt"
	"net/http"
	"os"
	"path"
	"strings"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3/s3manager"
)

// AwsCredentials is set at build time and contains the following csv format: "AccessKeyID,SecretAccessKey"
var AwsCredentials string

// logCollector implements functionality for uploading log files to the cloud
type logCollector struct {
	// only used to obtain user ID for log file generation
	tokener token.Accessor

	deviceID string

	bucketName   string
	s3BasePrefix string
	awsRegion    string

	AccessKeyID     string
	SecretAccessKey string

	httpClient *http.Client

	uploader *s3manager.Uploader
}

// New is a factory function for creating a file logger instance
func New(opts *options) (*logCollector, error) {
	creds := strings.Split(AwsCredentials, ",")

	if len(creds) != 2 {
		err := fmt.Errorf("Empty / Invalid format for AWS credentials (%d instead of 2 parts)", len(creds))
		return nil, err
	}

	c := &logCollector{
		tokener: opts.tokener,

		bucketName:   opts.bucketName,
		s3BasePrefix: opts.s3BasePrefix,
		awsRegion:    opts.awsRegion,

		AccessKeyID:     creds[0],
		SecretAccessKey: creds[1],

		httpClient: opts.httpClient,
	}

	if c.httpClient == nil {
		c.httpClient = http.DefaultClient
	}

	deviceID, err := robot.ReadESN()
	if err != nil {
		return nil, err
	}

	c.deviceID = deviceID

	// Create static AWS credentials (in the future we will be using STS credentials)
	awsSession, err := session.NewSession(&aws.Config{
		HTTPClient:  c.httpClient,
		Credentials: credentials.NewStaticCredentials(c.AccessKeyID, c.SecretAccessKey, ""),
		Region:      aws.String(c.awsRegion),
	})
	if err != nil {
		return nil, err
	}

	c.uploader = s3manager.NewUploader(awsSession)

	return c, nil
}

// Upload uploads file to cloud
func (c *logCollector) Upload(ctx context.Context, logFilePath string) (string, error) {
	// As the user ID may (theoretically) change we retrieve it here for every upload
	userID := c.tokener.UserID()
	if userID == "" {
		// Create a sensible fallback user ID for cloud uploads (in case no token is stored in file system)
		userID = "unknown-user-id"
	}

	timestamp := time.Now().UTC()

	s3Prefix := path.Join(c.s3BasePrefix, userID, c.deviceID)
	s3FileName := fmt.Sprintf("%s-%s", timestamp.Format("2006-01-02-15-04-05"), path.Base(logFilePath))
	s3Key := path.Join(s3Prefix, s3FileName)

	logFile, err := os.Open(logFilePath)
	if err != nil {
		return "", err
	}

	uploadInput := &s3manager.UploadInput{
		Bucket: aws.String(c.bucketName),
		Key:    aws.String(s3Key),
		Body:   logFile,
	}

	result, err := c.uploader.UploadWithContext(ctx, uploadInput)

	if err != nil {
		if awsErr, ok := err.(awserr.Error); ok {
			return "", fmt.Errorf("%s: %s (Bucket=%q, Key=%q)", awsErr.Code(), awsErr.Message(), c.bucketName, s3Key)
		}
		return "", fmt.Errorf("%v (Bucket=%q, Key=%q)", err, c.bucketName, s3Key)
	}

	log.Printf("File %q uploaded to %q\n", logFilePath, result.Location)

	return result.Location, nil
}
