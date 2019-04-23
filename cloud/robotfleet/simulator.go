package main

import (
	"fmt"
	"math/rand"
	"strings"
	"sync"
	"time"
)

type actionFunc func(action action) error

type action struct {
	id     string
	taskID int

	quitChan   chan struct{}
	actionFunc actionFunc

	metrics *actionMetrics
}

func (a action) reportAction(requestor string, latency time.Duration, err error) {
	// Note: as we can not send all metrics to Wavefront (this would exceed our quota)
	//       we typically subsample and therefore also send the latency data to Splunk
	logLatency(err, a.taskID, requestor, a.id, latency)

	a.metrics.reportActionMetrics(latency, err)
}

type periodicAction struct {
	action

	meanDuration   time.Duration
	stdDevDuration time.Duration
}

type oneShotAction struct {
	action

	delay time.Duration
}

func init() {
	rand.Seed(time.Now().UTC().UnixNano())
}

type SimulatorState int

const (
	SimulatorUnknown = iota
	SimulatorInitialized
	SimulatorStarting
	SimulatorStarted
	SimulatorStopping
	SimulatorStopped
	SimulatorTerminated
	SimulatorErrorStarting
	SimulatorErrorStopping
)

type simulator struct {
	mutex sync.Mutex

	state     SimulatorState
	startTime time.Time

	robotID   int
	taskID    int
	requestor string

	setupAction    *oneShotAction
	tearDownAction *oneShotAction

	metricsEnabled    bool
	actionRegistry    *actionMetricRegistry
	periodicActionMap map[string]*periodicAction
}

func newSimulator(actionRegistry *actionMetricRegistry, options *options, instanceOptions *instanceOptions) *simulator {
	return &simulator{
		state:             SimulatorInitialized,
		requestor:         instanceOptions.testUserName,
		robotID:           instanceOptions.robotID,
		taskID:            instanceOptions.taskID,
		metricsEnabled:    instanceOptions.taskID < *options.reportingTasks,
		actionRegistry:    actionRegistry,
		periodicActionMap: make(map[string]*periodicAction),
	}
}

func (s *simulator) set(name, value string) {
	parts := strings.Split(name, ".")
	if len(parts) != 2 {
		fmt.Printf("Error setting %q to %q, not conforming to \"action_id.key\" format (has %d instead of 2 parts)\n", name, value, len(parts))
		return
	}

	id := parts[0]
	key := parts[1]

	duration, err := time.ParseDuration(value)
	if err != nil {
		fmt.Printf("Error setting %q to %q, could not parse duration value %v\n", name, value, err)
		return
	}

	action := s.periodicActionMap[id]
	if action == nil {
		fmt.Printf("Error setting %q to %q, could not find action %q\n", name, value, id)
		return
	}

	switch key {
	case "mean":
		if action.meanDuration != duration {
			action.meanDuration = duration
			s.startPeriodicAction(action)
		}
	case "stddev":
		action.stdDevDuration = duration
	default:
		fmt.Printf("Error setting %q to %q, invalid key %q\n", name, value, key)
		return
	}
}

func (s *simulator) start() error {
	if !(s.state == SimulatorInitialized || s.state == SimulatorStopped) {
		return fmt.Errorf("starting from state %v not supported", s.state)
	}

	s.mutex.Lock()
	defer s.mutex.Unlock()

	s.state = SimulatorStarting

	s.startTime = time.Now()

	if s.setupAction != nil {
		s.setupAction.quitChan = make(chan struct{})

		go func(quitChan chan struct{}) {
			select {
			case <-time.After(s.setupAction.delay):
				s.mutex.Lock()

				start := time.Now()
				err := s.setupAction.actionFunc(s.setupAction.action)
				s.setupAction.reportAction(s.requestor, time.Since(start), err)
				if err != nil {
					s.state = SimulatorErrorStarting
					return
				}

				for _, action := range s.periodicActionMap {
					s.startPeriodicAction(action)
				}

				s.state = SimulatorStarted

				s.mutex.Unlock()
			case <-quitChan:
				return
			}
		}(s.setupAction.quitChan)
	} else {
		for _, action := range s.periodicActionMap {
			s.startPeriodicAction(action)
		}

		s.state = SimulatorStarted
	}

	return nil
}

func (s *simulator) stop() error {
	if s.state != SimulatorStarted && s.state != SimulatorStarting {
		return fmt.Errorf("stopping from state %v not supported", s.state)
	}

	s.mutex.Lock()
	defer s.mutex.Unlock()

	s.state = SimulatorStopping

	// stop any pending setup go routines (if any)
	if s.setupAction != nil && s.setupAction.quitChan != nil {
		close(s.setupAction.quitChan)
		s.setupAction.quitChan = nil
	}

	if s.tearDownAction != nil {
		s.tearDownAction.quitChan = make(chan struct{})

		go func(quitChan chan struct{}) {
			select {
			case <-time.After(s.tearDownAction.delay):
				s.mutex.Lock()

				for _, action := range s.periodicActionMap {
					s.stopPeriodicAction(action)
				}

				start := time.Now()
				err := s.tearDownAction.actionFunc(s.tearDownAction.action)
				s.tearDownAction.reportAction(s.requestor, time.Since(start), err)
				if err != nil {
					s.state = SimulatorErrorStopping
					return
				}

				s.state = SimulatorStopped

				s.mutex.Unlock()
			case <-quitChan:
				return
			}
		}(s.tearDownAction.quitChan)
	} else {
		for _, action := range s.periodicActionMap {
			s.stopPeriodicAction(action)
		}

		s.state = SimulatorStopped
	}

	return nil
}

func (s *simulator) quit() error {
	if s.state != SimulatorStopped {
		return fmt.Errorf("quiting from state %v not supported", s.state)
	}

	s.mutex.Lock()
	defer s.mutex.Unlock()

	// stop any ongoing setup
	if s.setupAction.quitChan != nil {
		close(s.setupAction.quitChan)
		s.setupAction.quitChan = nil
	}

	// stop any ongoing teardown
	if s.tearDownAction.quitChan != nil {
		close(s.tearDownAction.quitChan)
		s.tearDownAction.quitChan = nil
	}

	s.state = SimulatorTerminated

	return nil
}

func (s *simulator) addSetupAction(actionID string, delay time.Duration, actionFunc actionFunc) {
	s.mutex.Lock()
	defer s.mutex.Unlock()

	actionMetrics := s.actionRegistry.getActionMetrics(actionID)
	if actionMetrics == nil {
		actionMetrics = s.actionRegistry.registerActionMetrics(actionID, fmt.Sprintf("action.setup.%s", actionID), s.metricsEnabled)
	}

	s.setupAction = &oneShotAction{
		action: action{
			id:         actionID,
			taskID:     s.taskID,
			actionFunc: actionFunc,
			metrics:    actionMetrics,
		},
		delay: delay,
	}
}

func (s *simulator) addTearDownAction(actionID string, delay time.Duration, actionFunc actionFunc) {
	s.mutex.Lock()
	defer s.mutex.Unlock()

	actionMetrics := s.actionRegistry.getActionMetrics(actionID)
	if actionMetrics == nil {
		actionMetrics = s.actionRegistry.registerActionMetrics(actionID, fmt.Sprintf("action.teardown.%s", actionID), s.metricsEnabled)
	}

	s.tearDownAction = &oneShotAction{
		action: action{
			id:         actionID,
			taskID:     s.taskID,
			actionFunc: actionFunc,
			metrics:    actionMetrics,
		},
		delay: delay,
	}
}

func (s *simulator) addPeriodicAction(actionID string, meanDuration time.Duration, stdDevDuration time.Duration, actionFunc actionFunc) {
	s.mutex.Lock()
	defer s.mutex.Unlock()

	actionMetrics := s.actionRegistry.getActionMetrics(actionID)
	if actionMetrics == nil {
		actionMetrics = s.actionRegistry.registerActionMetrics(actionID, fmt.Sprintf("action.periodic.%s", actionID), s.metricsEnabled)
	}

	s.periodicActionMap[actionID] = &periodicAction{
		action: action{
			id:         actionID,
			taskID:     s.taskID,
			actionFunc: actionFunc,
			metrics:    actionMetrics,
		},
		meanDuration:   meanDuration,
		stdDevDuration: stdDevDuration,
	}
}

func calculateDuration(action *periodicAction, duration time.Duration) time.Duration {
	// calculate timer interval based on mean, standard deviation (while discounting duration of the action execution time)
	remainingDuration := time.Duration(rand.NormFloat64()*float64(action.stdDevDuration)+float64(action.meanDuration)) - duration

	if remainingDuration < 0 {
		return 0
	}

	return remainingDuration
}

// internal method, not used outside simulator type
func (s *simulator) startPeriodicAction(periodicAction *periodicAction) {
	// stop if already running
	s.stopPeriodicAction(periodicAction)

	if periodicAction.meanDuration == 0 {
		return
	}

	periodicAction.quitChan = make(chan struct{})

	go func() {
		var actionDuration time.Duration

		for {
			select {
			case <-time.After(calculateDuration(periodicAction, actionDuration)):
				start := time.Now()
				err := periodicAction.actionFunc(periodicAction.action)
				periodicAction.reportAction(s.requestor, time.Since(start), err)
			case <-periodicAction.quitChan:
				return
			}
		}
	}()
}

// internal method, not used outside simulator type
func (s *simulator) stopPeriodicAction(periodicAction *periodicAction) {
	if periodicAction.quitChan != nil {
		close(periodicAction.quitChan)
		periodicAction.quitChan = nil
	}
}
