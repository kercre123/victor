package main

import (
	"time"
)

type testSimulator struct {
	*simulator

	pollingInterval time.Duration
	numRetries      int

	setupDone    bool
	tearDownDone bool
	actionCount1 int
	actionCount2 int
}

func newTestSimulator() *testSimulator {
	reportingTasks := 0
	options := &options{reportingTasks: &reportingTasks}

	instanceOptions := &instanceOptions{taskID: 0, robotID: 0, testUserName: "test"}

	return &testSimulator{
		pollingInterval: time.Millisecond * 20,
		numRetries:      5,
		simulator:       newSimulator(newActionMetricRegistry(), options, instanceOptions),
	}
}

func (s *testSimulator) setupAction(action action) error {
	s.setupDone = true
	return nil
}

func (s *testSimulator) tearDownAction(action action) error {
	s.tearDownDone = true
	return nil
}

func (s *testSimulator) periodicAction1(action action) error {
	s.actionCount1++
	return nil
}

func (s *testSimulator) periodicAction2(action action) error {
	s.actionCount2++
	return nil
}

func (s *testSimulator) periodicAction3(action action) error {
	return nil
}

func (s *testSimulator) actionDone1() bool {
	return s.actionCount1 > 0
}

func (s *testSimulator) actionDone2() bool {
	return s.actionCount2 > 0
}

func (s *testSimulator) allActionsDone() bool {
	return s.actionDone1() && s.actionDone2()
}

func (s *testSimulator) waitForState(expectedState SimulatorState) bool {
	for retries := s.numRetries; retries > 0 && s.state != expectedState; retries-- {
		time.Sleep(s.pollingInterval)
	}

	return s.state == expectedState
}

func (s *testSimulator) waitForReriodicActions() bool {
	for retries := s.numRetries; retries > 0 && !s.allActionsDone(); retries-- {
		time.Sleep(s.pollingInterval)
	}

	return s.allActionsDone()
}
