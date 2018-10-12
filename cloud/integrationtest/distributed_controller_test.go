package main

import (
	"fmt"
	"math"
	"testing"
	"time"

	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
)

type testSimulator struct {
	*simulator

	setupDone    bool
	tearDownDone bool
	actionDone1  bool
	actionDone2  bool
}

func newTestSimulator() *testSimulator {
	return &testSimulator{simulator: newSimulator()}
}

func (s *testSimulator) setupAction() error {
	s.setupDone = true
	return nil
}

func (s *testSimulator) tearDownAction() error {
	s.tearDownDone = true
	return nil
}

func (s *testSimulator) periodicAction1() error {
	s.actionDone1 = true
	return nil
}

func (s *testSimulator) periodicAction2() error {
	s.actionDone2 = true
	return nil
}

func (s *testSimulator) periodicAction3() error {
	return nil
}

type DistributedControllerTestSuite struct {
	suite.Suite

	pollingInterval time.Duration
	numRetries      int

	controller *distributedController
	simulator  *testSimulator
}

func (s *DistributedControllerTestSuite) SetupSuite() {
	s.pollingInterval = time.Millisecond * 20
	s.numRetries = 5

	s.controller = newDistributedController("localhost:6379")

	s.simulator = newTestSimulator()

	s.simulator.addSetupAction(s.simulator.setupAction)
	s.simulator.addTearDownAction(s.simulator.tearDownAction)
	s.simulator.addPeriodicAction("action1", time.Millisecond*50, 0, s.simulator.periodicAction1)
	s.simulator.addPeriodicAction("action2", time.Millisecond*100, 0, s.simulator.periodicAction2)
	s.simulator.addPeriodicAction("action3", 0, 0, s.simulator.periodicAction3)

	s.controller.forwardCommands(s.simulator)
}

func (s *DistributedControllerTestSuite) waitForState(expectedState State) bool {
	for retries := s.numRetries; retries > 0 && s.controller.state != expectedState; retries-- {
		time.Sleep(s.pollingInterval)
	}

	return s.controller.state == expectedState
}

func (s *DistributedControllerTestSuite) waitForReriodicActions() bool {
	for retries := s.numRetries; retries > 0 && (!s.simulator.actionDone1 || !s.simulator.actionDone2); retries-- {
		time.Sleep(s.pollingInterval)
	}

	return s.simulator.actionDone1 && s.simulator.actionDone2
}

func (s *DistributedControllerTestSuite) TestIDIncrementer() {
	id1, err := s.controller.provideUniqueTestID()
	s.NoError(err)

	id2, err := s.controller.provideUniqueTestID()
	s.NoError(err)

	s.Equal(id2, id1+1)
}

func (s *DistributedControllerTestSuite) TestDurationDistribution() {
	const numSamples = 1000

	action := &action{meanDuration: time.Millisecond * 100, stdDevDuration: time.Millisecond * 50}

	var totalDuration time.Duration
	for i := 0; i < numSamples; i++ {
		duration := s.simulator.calculateDuration(action, 0)
		totalDuration += duration
		s.True(duration >= 0)
	}

	s.True(math.Abs(float64(action.meanDuration-totalDuration/numSamples)) < float64(action.stdDevDuration))
}

func (s *DistributedControllerTestSuite) TestSetKeyValue() {
	require := require.New(s.T())

	const id = "action3"
	const duration = "10s"

	expectedDuration, err := time.ParseDuration(duration)
	require.NoError(err)

	action := s.simulator.periodicActionMap[id]
	require.NotNil(action)

	s.Equal(time.Duration(0), action.stdDevDuration)
	s.Equal(time.Duration(0), action.meanDuration)

	// set standard deviation
	s.controller.sendCommand(fmt.Sprintf("set:%s.stddev=%s", id, duration))

	// allow for round-trip to external Redis
	time.Sleep(time.Millisecond * 50)

	s.Equal(expectedDuration, action.stdDevDuration)
	s.Equal(time.Duration(0), action.meanDuration)

	// set (mean) interval duration
	s.controller.sendCommand(fmt.Sprintf("set:%s.mean=%s", id, duration))

	// allow for round-trip to external Redis
	time.Sleep(time.Millisecond * 50)

	s.Equal(expectedDuration, action.stdDevDuration)
	s.Equal(expectedDuration, action.meanDuration)
}

func (s *DistributedControllerTestSuite) TestStartStop() {
	s.False(s.simulator.setupDone)
	s.False(s.simulator.tearDownDone)
	s.False(s.simulator.actionDone1)
	s.False(s.simulator.actionDone2)
	s.Equal(Initialized, s.controller.state)

	s.controller.sendCommand("start")

	// allow periodic timers to be executed
	s.waitForState(Started)
	s.Equal(Started, s.controller.state)

	s.waitForReriodicActions()

	s.True(s.simulator.setupDone)
	s.False(s.simulator.tearDownDone)
	s.True(s.simulator.actionDone1)
	s.True(s.simulator.actionDone2)

	s.controller.sendCommand("stop")

	// allow tear down to be executed
	s.waitForState(Stopped)
	s.Equal(Stopped, s.controller.state)

	s.True(s.simulator.setupDone)
	s.True(s.simulator.tearDownDone)
	s.True(s.simulator.actionDone1)
	s.True(s.simulator.actionDone2)

	s.controller.sendCommand("quit")

	// allow quit to be executed
	s.waitForState(Terminated)
	s.Equal(Terminated, s.controller.state)
}

func TestDistributedControllerTestSuite(t *testing.T) {
	suite.Run(t, new(DistributedControllerTestSuite))
}
