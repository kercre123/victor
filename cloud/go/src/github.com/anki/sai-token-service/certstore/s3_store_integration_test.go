// +build integration

package certstore

import (
	"context"
	"io/ioutil"
	"testing"

	"github.com/anki/sai-go-util/dockerutil"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
)

const (
	testBucket = "test"
	testPrefix = "/test"
)

type S3RobotCertificateStoreSuite struct {
	dockerutil.S3Suite
	Store RobotCertificateStore
}

func TestS3RobotCertificateStoreSuite(t *testing.T) {
	suite.Run(t, new(S3RobotCertificateStoreSuite))
}

func (s *S3RobotCertificateStoreSuite) SetupTest() {
	s.CreateBucket(testBucket)
	store, err := NewS3RobotCertificateStore(testBucket, testPrefix, s.Config)
	if err != nil {
		s.T().Fatalf("Failed to initialize S3RobotCertificateStore: %s", err)
	}
	s.Store = store
}

func (s *S3RobotCertificateStoreSuite) TearDownTest() {
	s.DeleteAllObjects(testBucket)
	s.DeleteBucket(testBucket)
	s.Store = nil
}

func (s *S3RobotCertificateStoreSuite) TestSimpleStore() {
	t := s.T()
	cert, err := ioutil.ReadFile("testdata/cert1.pem")
	require.NoError(t, err)
	assert.Len(t, s.ListObjects(testBucket, testPrefix), 0)
	require.NoError(t, s.Store.Store(context.Background(), "vic", "robot", cert))
	assert.Len(t, s.ListObjects(testBucket, testPrefix), 1)
	resp, data := s.GetObjectWithResponse(testBucket, testPrefix+"/vic/robot")
	assert.Equal(t, aws.StringValue(resp.CacheControl), "no-cache,no-store,max-age=0,must-revalidate")
	assert.Equal(t, cert, data)
}

func (s *S3RobotCertificateStoreSuite) TestMultipleStore() {
	t := s.T()
	cert1, err := ioutil.ReadFile("testdata/cert1.pem")
	require.NoError(t, err)
	cert2, err := ioutil.ReadFile("testdata/cert2.pem")
	require.NoError(t, err)
	require.NotEqual(t, cert1, cert2)
	assert.Len(t, s.ListObjects(testBucket, testPrefix), 0)
	require.NoError(t, s.Store.Store(context.Background(), "vic", "robot1", cert1))
	assert.Len(t, s.ListObjects(testBucket, testPrefix), 1)
	require.NoError(t, s.Store.Store(context.Background(), "vic", "robot2", cert2))
	assert.Len(t, s.ListObjects(testBucket, testPrefix), 2)

	assert.Equal(t, cert1, s.GetObject(testBucket, testPrefix+"/vic/robot1"))
	assert.Equal(t, cert2, s.GetObject(testBucket, testPrefix+"/vic/robot2"))
}

func (s *S3RobotCertificateStoreSuite) TestInvalidPEM() {
	t := s.T()
	cert := []byte("this is not a certificate")
	require.Error(t, s.Store.Store(context.Background(), "vic", "robot", cert))
	require.Equal(t, errorInvalidPEM, validate(cert))
}

func (s *S3RobotCertificateStoreSuite) TestValidPEMWithNoCertificate() {
	t := s.T()
	key, err := ioutil.ReadFile("testdata/key.pem")
	require.NoError(t, err)
	require.Error(t, s.Store.Store(context.Background(), "vic", "robot", key))
}
