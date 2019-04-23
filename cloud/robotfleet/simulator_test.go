package main

import (
	"fmt"
	"math"
	"testing"
	"time"

	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"
)

type SimulatorTestSuite struct {
	suite.Suite
}

func (s *SimulatorTestSuite) TestDurationDistribution() {
	const numSamples = 1000

	action := &periodicAction{meanDuration: time.Millisecond * 100, stdDevDuration: time.Millisecond * 50}

	var totalDuration time.Duration
	for i := 0; i < numSamples; i++ {
		duration := calculateDuration(action, 0)
		totalDuration += duration
		s.True(duration >= 0)
	}

	s.True(math.Abs(float64(action.meanDuration-totalDuration/numSamples)) < float64(action.stdDevDuration))
}

func (s *SimulatorTestSuite) TestSetKeyValue() {
	require := require.New(s.T())

	const id = "action3"
	const duration = "10s"

	expectedDuration, err := time.ParseDuration(duration)
	require.NoError(err)

	simulator := newTestSimulator()

	simulator.addPeriodicAction(id, 0, 0, nil)

	action := simulator.periodicActionMap[id]
	require.NotNil(action)

	// initially we expect parameters to be 0
	s.Equal(time.Duration(0), action.stdDevDuration)
	s.Equal(time.Duration(0), action.meanDuration)

	// set standard deviation
	simulator.set(fmt.Sprintf("%s.stddev", id), duration)

	s.Equal(expectedDuration, action.stdDevDuration)
	s.Equal(time.Duration(0), action.meanDuration)

	// set (mean) interval duration
	simulator.set(fmt.Sprintf("%s.mean", id), duration)

	s.Equal(expectedDuration, action.stdDevDuration)
	s.Equal(expectedDuration, action.meanDuration)
}

func (s *SimulatorTestSuite) TestStartStop() {
	require := require.New(s.T())

	simulator := newTestSimulator()

	simulator.addSetupAction("setup", 0, simulator.setupAction)
	simulator.addTearDownAction("teardown", 0, simulator.tearDownAction)
	simulator.addPeriodicAction("action1", time.Millisecond*50, 0, simulator.periodicAction1)
	simulator.addPeriodicAction("action2", time.Millisecond*100, 0, simulator.periodicAction2)

	s.Equal(SimulatorState(SimulatorInitialized), simulator.state)

	s.False(simulator.setupDone)
	s.False(simulator.tearDownDone)
	s.False(simulator.actionDone1())
	s.False(simulator.actionDone2())

	// stopping allowed only when started
	err := simulator.stop()
	s.Error(err)

	err = simulator.start()
	require.NoError(err)

	// allow periodic timers to be executed
	simulator.waitForState(SimulatorStarted)
	s.Equal(SimulatorState(SimulatorStarted), simulator.state)

	simulator.waitForReriodicActions()

	s.True(simulator.setupDone)
	s.False(simulator.tearDownDone)
	s.True(simulator.actionDone1())
	s.True(simulator.actionDone2())

	err = simulator.stop()
	require.NoError(err)

	// allow tear down to be executed
	simulator.waitForState(SimulatorStopped)
	s.Equal(SimulatorState(SimulatorStopped), simulator.state)
	time.Sleep(1 * time.Second)

	s.True(simulator.setupDone)
	s.True(simulator.tearDownDone)
	s.True(simulator.actionDone1())
	s.True(simulator.actionDone2())

	err = simulator.quit()
	require.NoError(err)

	// allow quit to be executed
	simulator.waitForState(SimulatorTerminated)
	s.Equal(SimulatorState(SimulatorTerminated), simulator.state)
}

func (s *SimulatorTestSuite) TestDelayedStart() {
	require := require.New(s.T())

	simulator := newTestSimulator()

	simulator.addSetupAction("setup", time.Millisecond*50, simulator.setupAction)

	err := simulator.start()
	require.NoError(err)

	s.False(simulator.setupDone)

	time.Sleep(time.Millisecond * 30)
	s.Equal(SimulatorState(SimulatorStarting), simulator.state)

	s.False(simulator.setupDone)

	simulator.waitForState(SimulatorStarted)
	s.Equal(SimulatorState(SimulatorStarted), simulator.state)

	s.True(simulator.setupDone)
}

func (s *SimulatorTestSuite) TestQuitBeforeStartDelay() {
	require := require.New(s.T())

	simulator := newTestSimulator()

	simulator.addSetupAction("setup", time.Hour, simulator.setupAction)

	err := simulator.start()
	require.NoError(err)

	simulator.waitForState(SimulatorStarting)
	s.Equal(SimulatorState(SimulatorStarting), simulator.state)

	// allow start go routing to be scheduled
	time.Sleep(time.Millisecond * 100)

	err = simulator.stop()
	require.NoError(err)

	simulator.waitForState(SimulatorStopped)
	s.Equal(SimulatorState(SimulatorStopped), simulator.state)
}

func (s *SimulatorTestSuite) TestStartStopPerAction() {
	require := require.New(s.T())

	simulator := newTestSimulator()

	simulator.addPeriodicAction("action1", 0, 0, simulator.periodicAction1)
	simulator.addPeriodicAction("action2", 0, 0, simulator.periodicAction2)

	err := simulator.start()
	require.NoError(err)

	simulator.waitForState(SimulatorStarted)
	s.Equal(SimulatorState(SimulatorStarted), simulator.state)

	s.False(simulator.actionDone1())
	s.False(simulator.actionDone2())

	time.Sleep(time.Millisecond * 30)

	s.False(simulator.actionDone1())
	s.False(simulator.actionDone2())

	simulator.set("action1.mean", "10ms")

	time.Sleep(time.Millisecond * 20)

	s.True(simulator.actionDone1())
	s.False(simulator.actionDone2())

	simulator.set("action1.mean", "0s")
	simulator.set("action2.mean", "10ms")

	time.Sleep(time.Millisecond * 20)

	s.True(simulator.allActionsDone())

	err = simulator.stop()
	require.NoError(err)

	simulator.waitForState(SimulatorStopped)
	s.Equal(SimulatorState(SimulatorStopped), simulator.state)
}

func TestSimulator(t *testing.T) {
	suite.Run(t, new(SimulatorTestSuite))
}
