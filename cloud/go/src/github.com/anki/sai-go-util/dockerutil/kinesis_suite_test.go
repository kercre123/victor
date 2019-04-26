// +build integration

package dockerutil

import (
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"
	"testing"
)

type KinesisSuite_TestSuite struct {
	KinesisSuite
}

func TestKinesisSuite(t *testing.T) {
	suite.Run(t, new(KinesisSuite_TestSuite))
}

func (s *KinesisSuite_TestSuite) TestHelpers() {
	// Create a stream
	shardID := s.KinesisSuite.CreateStream("test")
	assert.NotEmpty(s.T(), shardID)

	recsInput := [][]byte{
		[]byte{1, 2},
		[]byte{3, 4},
	}
	s.KinesisSuite.PutRecords("test", recsInput)

	recs := s.KinesisSuite.GetRecords("test", shardID)
	assert.Equal(s.T(), recsInput, recs)

	s.KinesisSuite.DeleteStream("test")
}
