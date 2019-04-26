// +build integration

package dockerutil

import (
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"
	"testing"
)

type SQSSuite_TestSuite struct {
	SQSSuite
}

func TestSQSSuite(t *testing.T) {
	suite.Run(t, new(SQSSuite_TestSuite))
}

func (s *SQSSuite_TestSuite) TestHelpers() {
	url := s.SQSSuite.CreateQueue("test")
	assert.NotEmpty(s.T(), url)
	s.SQSSuite.DeleteQueue(url)
}
