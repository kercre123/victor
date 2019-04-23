package main

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
)

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

	s.simulator.addSetupAction("setup", 0, s.simulator.setupAction)
	s.simulator.addTearDownAction("teardown", 0, s.simulator.tearDownAction)
	s.simulator.addPeriodicAction("action1", time.Millisecond*50, 0, s.simulator.periodicAction1)
	s.simulator.addPeriodicAction("action2", time.Millisecond*100, 0, s.simulator.periodicAction2)
	s.simulator.addPeriodicAction("action3", 0, 0, s.simulator.periodicAction3)

	s.controller.forwardCommands(s.simulator)
}

func (s *DistributedControllerTestSuite) TaskIDIncrementer() {
	id1, err := s.controller.uniqueTaskID()
	s.NoError(err)

	id2, err := s.controller.uniqueTaskID()
	s.NoError(err)

	s.Equal(id2, id1+1)
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
	s.False(s.simulator.actionDone1())
	s.False(s.simulator.actionDone2())
	s.Equal(SimulatorState(SimulatorInitialized), s.simulator.state)

	s.controller.sendCommand("start")

	// allow periodic timers to be executed
	s.simulator.waitForState(SimulatorStarted)
	s.Equal(SimulatorState(SimulatorStarted), s.simulator.state)

	s.simulator.waitForReriodicActions()

	s.True(s.simulator.setupDone)
	s.False(s.simulator.tearDownDone)
	s.True(s.simulator.actionDone1())
	s.True(s.simulator.actionDone2())

	s.controller.sendCommand("stop")

	// allow tear down to be executed
	s.simulator.waitForState(SimulatorStopped)
	s.Equal(SimulatorState(SimulatorStopped), s.simulator.state)

	s.True(s.simulator.setupDone)
	s.True(s.simulator.tearDownDone)
	s.True(s.simulator.actionDone1())
	s.True(s.simulator.actionDone2())

	s.controller.sendCommand("quit")

	// allow quit to be executed
	s.simulator.waitForState(SimulatorTerminated)
	s.Equal(SimulatorState(SimulatorTerminated), s.simulator.state)
}

func TestDistributedController(t *testing.T) {
	suite.Run(t, new(DistributedControllerTestSuite))
}
