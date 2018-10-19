package main

import (
	"fmt"
	"math/rand"
	"strings"
	"syscall"
	"time"
)

type actionFunc func() error

type action struct {
	id             string
	quitChan       chan struct{}
	meanDuration   time.Duration
	stdDevDuration time.Duration
	action         actionFunc
}

func init() {
	rand.Seed(time.Now().UTC().UnixNano())
}

func (a action) isRunning() bool {
	return a.quitChan != nil
}

type simulator struct {
	started bool

	setupActionFunc    actionFunc
	tearDownActionFunc actionFunc

	periodicActionMap map[string]*action
}

func newSimulator() *simulator {
	return &simulator{periodicActionMap: make(map[string]*action)}
}

func (s *simulator) start() {
	if s.started {
		return
	}

	s.started = true

	if s.setupActionFunc != nil {
		s.setupActionFunc()
	}

	for _, action := range s.periodicActionMap {
		s.startPeriodicAction(action)
	}
}

func (s *simulator) stop() {
	if !s.started {
		return
	}

	for _, action := range s.periodicActionMap {
		s.stopPeriodicAction(action)
	}

	if s.tearDownActionFunc != nil {
		s.tearDownActionFunc()
	}

	s.started = false
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
		action.meanDuration = duration
		if !action.isRunning() && duration > 0 {
			// this action requires starting (was not running)
			s.startPeriodicAction(action)
		} else if action.isRunning() && duration == 0 {
			// this action requires stopping (was running)
			s.stopPeriodicAction(action)
		}
	case "stddev":
		action.stdDevDuration = duration
	default:
		fmt.Printf("Error setting %q to %q, invalid key %q\n", name, value, key)
		return
	}
}

func (s *simulator) quit() {
	s.stop()

	signalChannel <- syscall.SIGINT
}

func (s *simulator) addSetupAction(action actionFunc) {
	s.setupActionFunc = action
}

func (s *simulator) addTearDownAction(action actionFunc) {
	s.tearDownActionFunc = action
}

func (s *simulator) addPeriodicAction(id string, meanDuration time.Duration, stdDevDuration time.Duration, periodicAction actionFunc) {
	fmt.Printf("Initializing periodic timer %q (interval=%v, stddev=%v)\n", id, meanDuration, stdDevDuration)

	s.periodicActionMap[id] = &action{
		id:             id,
		meanDuration:   meanDuration,
		stdDevDuration: stdDevDuration,
		action:         periodicAction,
	}
}

func (*simulator) calculateDuration(action *action, duration time.Duration) time.Duration {
	// calculate timer interval based on mean, standard deviation (while discounting duration of the action execution time)
	remainingDuration := time.Duration(rand.NormFloat64()*float64(action.stdDevDuration)+float64(action.meanDuration)) - duration

	if remainingDuration < 0 {
		return 0
	}

	return remainingDuration
}

func (s *simulator) startPeriodicAction(action *action) {
	if !s.started || action.meanDuration == 0 {
		// no need to do anything
		return
	}

	if action.isRunning() {
		fmt.Printf("Periodic action %q already running\n", action.id)
		return
	}

	action.quitChan = make(chan struct{})

	go func() {
		var actionDuration time.Duration

	timerloop:
		for {
			select {
			case <-time.After(s.calculateDuration(action, actionDuration)):
				start := time.Now()
				action.action()
				actionDuration = time.Since(start)
			case <-action.quitChan:
				break timerloop
			}
		}
	}()
}

func (s *simulator) stopPeriodicAction(action *action) {
	if !s.started {
		// no need to do anything
		return
	}

	if !action.isRunning() {
		fmt.Printf("Periodic action %q not running\n", action.id)
		return
	}

	close(action.quitChan)
	action.quitChan = nil
}
