// +build integration

package dockerutil

import (
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"
	"testing"
)

// test suite to test the helper functions in the S3 test suite base
// type
type S3Suite_TestSuite struct {
	S3Suite
}

func TestS3Suite(t *testing.T) {
	suite.Run(t, new(S3Suite_TestSuite))
}

func (s *S3Suite_TestSuite) TestHelpers() {
	// Create a test bucket
	s.S3Suite.CreateBucket("test")

	// Upload an object
	s.S3Suite.PutObject("test", "path", "application/binary", []byte{1, 2})

	// Verify that GetObject returns the same data
	assert.Equal(s.T(), []byte{1, 2}, s.S3Suite.GetObject("test", "path"))

	// Add 2 more objects
	s.S3Suite.PutObject("test", "path2", "application/binary", []byte{1, 2})
	s.S3Suite.PutObject("test", "path3", "application/binary", []byte{1, 2})

	// List all objects with the same prefix and verify the right number
	objs := s.S3Suite.ListObjects("test", "path")
	assert.Equal(s.T(), 3, len(objs))

	// Delete one of them
	s.S3Suite.DeleteObject("test", "path")

	// List again, verifying the correct number
	objs = s.S3Suite.ListObjects("test", "")
	assert.Equal(s.T(), 2, len(objs))

	// Delete all remaining objects
	s.S3Suite.DeleteAllObjects("test")

	// verify no objects remain
	objs = s.S3Suite.ListObjects("test", "")
	assert.Equal(s.T(), 0, len(objs))

	// Delete the bucket
	s.S3Suite.DeleteBucket("test")
}
