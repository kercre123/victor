package main

import (
	"time"
)

type actionFunc func() error

type action struct {
	quitChan chan struct{}
	action   actionFunc
}

type simulator struct {
	setupActionFunc    actionFunc
	tearDownActionFunc actionFunc

	periodicActions []*action
}

func (s *simulator) start() {
	if s.setupActionFunc != nil {
		s.setupActionFunc()
	}
}

func (s *simulator) stop() {
	for _, action := range s.periodicActions {
		close(action.quitChan)
	}

	if s.tearDownActionFunc != nil {
		s.tearDownActionFunc()
	}
}

func (s *simulator) addSetupAction(action actionFunc) {
	s.setupActionFunc = action
}

func (s *simulator) addTearDownAction(action actionFunc) {
	s.tearDownActionFunc = action
}

func (s *simulator) addPeriodicAction(period time.Duration, periodicAction actionFunc) {
	if period == 0 {
		return
	}

	ticker := time.NewTicker(period)

	quit := make(chan struct{})
	s.periodicActions = append(s.periodicActions, &action{quitChan: quit, action: periodicAction})

	go func() {
		for {
			select {
			case <-ticker.C:
				periodicAction()
			case <-quit:
				ticker.Stop()
			}
		}
	}()
}
