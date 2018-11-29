package logcollector

import (
	"anki/token/identity"
	"context"
	"encoding/base64"
	"fmt"
	"io/ioutil"
	"math/rand"
	"net/url"
	"os"
	"strings"
	"testing"
	"time"

	"github.com/anki/sai-go-util/dockerutil"
	ac "github.com/aws/aws-sdk-go/aws/credentials"
	"google.golang.org/grpc/credentials"
	gc "google.golang.org/grpc/credentials"

	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
)

const (
	accessKey      = "AKIAIOSFODNN7EXAMPLE"
	secretKey      = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY"
	region         = "us-east-1"
	testBucketName = "logbucket"
	testPathPrefix = "pathprefix"
	testUserID     = "test-user-id"
	testDeviceID   = "00000000"
)

// IdentityProvider implementation that allows for testing without real backend
type TestIdentityProvider struct{}

func (i TestIdentityProvider) Init() error {
	return nil
}

func (i TestIdentityProvider) ParseAndStoreToken(token string) (identity.Token, error) {
	return nil, nil
}

func (i TestIdentityProvider) TransportCredentials() credentials.TransportCredentials {
	return nil
}

func (i TestIdentityProvider) CertCommonName() string {
	return testDeviceID
}

func (i TestIdentityProvider) GetToken() identity.Token {
	return nil
}

// Accessor implementation that allows for testing without real token service
type TestTokener struct{}

func (t TestTokener) Credentials() (gc.PerRPCCredentials, error) {
	return nil, nil
}

func (t TestTokener) UserID() string {
	return testUserID
}

func (t TestTokener) GetStsCredentials() (*ac.Credentials, error) {
	return ac.NewStaticCredentialsFromCreds(ac.Value{
		AccessKeyID:     accessKey,
		SecretAccessKey: secretKey,
	}), nil
}

func (t TestTokener) IdentityProvider() identity.Provider {
	return TestIdentityProvider{}
}

func newTestLogCollector(endpoint, bucketName string) (*logCollector, error) {
	logcollectorOpts := []Option{
		WithServer(),
		WithEndpoint(endpoint),
		WithS3ForcePathStyle(true),
		WithS3UrlPrefix(fmt.Sprintf("s3://%s/%s", bucketName, testPathPrefix)),
		WithDisableSSL(true),
		WithAwsRegion(region),
	}

	var opts options
	for _, o := range logcollectorOpts {
		o(&opts)
	}

	return newLogCollector(TestTokener{}, &opts)
}

type LogCollectorSuite struct {
	dockerutil.S3Suite

	testRunID    int
	testFilePath string
}

func (s *LogCollectorSuite) SetupSuite() {
	rand.Seed(time.Now().UnixNano())
	s.testRunID = rand.Intn(100000)

	s.AccessKey = accessKey
	s.SecretKey = secretKey

	s.S3Suite.SetupSuite()

	s.CreateBucket(testBucketName)

	s.createTestLogFile(fmt.Sprintf("test_run_%d\n", s.testRunID))
}

func (s *LogCollectorSuite) TearDownSuite() {
	s.S3Suite.TearDownSuite()
}

func (s *LogCollectorSuite) createTestLogFile(content string) error {
	file, err := ioutil.TempFile(os.TempDir(), "logcollector_test_")
	if err != nil {
		return err
	}
	defer file.Close()

	s.testFilePath = file.Name()

	_, err = file.WriteString(content)
	if err != nil {
		return err
	}

	return nil
}

func (s *LogCollectorSuite) TestUploadNoServer() {
	t := s.T()

	collector, err := newTestLogCollector("http://1.2.3.4:12345", testBucketName)
	require.NoError(t, err)

	s3Url, err := collector.Upload(context.Background(), testBucketName)
	s.Error(err)
	s.Empty(s3Url)
}

func (s *LogCollectorSuite) TestUploadNonExistingLogFile() {
	t := s.T()

	collector, err := newTestLogCollector(s.Container.Addr(), testBucketName)
	require.NoError(t, err)

	s3Url, err := collector.Upload(context.Background(), "/non/existing/file.log")
	s.Error(err)
	s.Equal("open /non/existing/file.log: no such file or directory", err.Error())
	s.Empty(s3Url)
}

func (s *LogCollectorSuite) TestUploadNonExistingBucketName() {
	t := s.T()

	collector, err := newTestLogCollector(s.Container.Addr(), "non-existing-bucket")
	require.NoError(t, err)

	s3Url, err := collector.Upload(context.Background(), s.testFilePath)

	s.Error(err)
	s.True(strings.HasPrefix(err.Error(), "NoSuchBucket: The specified bucket does not exist"))
	s.Empty(s3Url)
}

func (s *LogCollectorSuite) TestUpload() {
	t := s.T()

	collector, err := newTestLogCollector(s.Container.Addr(), testBucketName)
	require.NoError(t, err)

	s3Url, err := collector.Upload(context.Background(), s.testFilePath)
	require.NoError(t, err)

	parsedURL, err := url.Parse(s3Url)
	require.NoError(t, err)
	s.NotEmpty(s3Url)

	s.Equal(s.Container.Addr(), parsedURL.Host)

	segments := strings.Split(parsedURL.Path[1:], "/")
	encodedTestDeviceId := base64.StdEncoding.EncodeToString([]byte(testDeviceID))
	s.Equal([]string{testBucketName, testPathPrefix, testUserID, encodedTestDeviceId}, segments[0:4])

	key := strings.Join(segments[1:], "/")
	content := s.GetObject(testBucketName, key)

	s.Equal(fmt.Sprintf("test_run_%d\n", s.testRunID), string(content))
}

func TestSuite(t *testing.T) {
	suite.Run(t, new(LogCollectorSuite))
}
