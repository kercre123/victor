// +build integration

package dockerutil

import (
	"github.com/stretchr/testify/suite"
	"testing"
)

// test suite to test the helper functions in the Dynamo test suite base
// type
type DynamoSuite_TestSuite struct {
	DynamoSuite
}

// Right now this just makes sure that the dynamo container launches
// correctly. When helper functions are added to the suite they will
// be tested here.
func TestDynamoSuite(t *testing.T) {
	suite.Run(t, new(DynamoSuite_TestSuite))
}
