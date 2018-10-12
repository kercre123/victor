package main

import (
	"fmt"
	"syscall"
	"time"
)

type actionFunc func() error

type action struct {
	id       string
	quitChan chan struct{}
	period   time.Duration
	action   actionFunc
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

	for id, action := range s.periodicActionMap {
		if action.quitChan != nil {
			close(action.quitChan)
			action.quitChan = nil
		} else {
			fmt.Printf("Periodic action %q not running\n", id)
		}
	}

	if s.tearDownActionFunc != nil {
		s.tearDownActionFunc()
	}

	s.started = false
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

func (s *simulator) addPeriodicAction(id string, period time.Duration, periodicAction actionFunc) {
	if period == 0 {
		return
	}

	s.periodicActionMap[id] = &action{
		id:     id,
		period: period,
		action: periodicAction,
	}
}

func (s *simulator) startPeriodicAction(action *action) {
	if !s.started {
		return
	}

	if action.quitChan != nil {
		fmt.Printf("Periodic action %q already running\n", action.id)
		return
	}

	action.quitChan = make(chan struct{})

	ticker := time.NewTicker(action.period)

	go func() {
	timerloop:
		for {
			select {
			case <-ticker.C:
				action.action()
			case <-action.quitChan:
				ticker.Stop()
				break timerloop
			}
		}
	}()
}
