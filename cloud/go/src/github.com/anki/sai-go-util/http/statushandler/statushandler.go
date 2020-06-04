// Copyright 2016 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// Package statushandler provides a handler for a /status endpoint along with a
// utility type that can collect status information at regular intervals and
// flip the status from ok  to failure after a number of successive failures.
package statushandler

import (
	"encoding/json"
	"errors"
	"net/http"
	"os"
	"time"

	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/log"
)

var (
	slowCheckDuration = time.Second
)

// A StatusChecker can be used with a StatusHandler to provide a value for the
// status field.
type StatusChecker interface {
	IsHealthy() error
}

// StatusCheckerFunc is an adapter type to turn an ordinary function
// into a StatusChecker.
type StatusCheckerFunc func() error

// IsHealthy calls f().
func (f StatusCheckerFunc) IsHealthy() error {
	return f()
}

// StatusHandler provides an http.Handler interface for a /status endpoint.
// It returns build information in JSON format read from the package level
// Build* variables and queries the registered StatusChecker to determine
// whether a status field should have a value of "ok" or "error".
//
// Returns 200 on an ok status, or 500 if the checker returns error.
type StatusHandler struct {
	StatusChecker StatusChecker
}

// ServeHTTP implements the http.Handler interface.
func (s StatusHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	var (
		ok        bool
		status    string
		lastCheck time.Time
	)
	if s.StatusChecker != nil {
		// false by default
		ok = s.StatusChecker.IsHealthy() == nil
		lastCheck = time.Now()
	}

	hostname, err := os.Hostname()
	if err != nil {
		hostname = "unknown"
	}

	if ok {
		status = "ok"
	} else {
		status = "error"
	}

	response := map[string]interface{}{
		"hostname":   hostname,
		"status":     status,
		"last_check": lastCheck.Format(time.RFC3339),
	}

	for k, v := range buildinfo.Map() {
		response[k] = v
	}

	jdata, err := json.Marshal(response)
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte("Failed to encode status to json :-("))
		return
	}

	w.Header().Set("Content-Type", "text/json")
	if ok {
		w.WriteHeader(http.StatusOK)
	} else {
		w.WriteHeader(http.StatusInternalServerError)
	}
	w.Write(jdata)
}

// StatusMonitor periodically checks connectivity with the database
// and complies with the StatusChecker interface.
//
// If a number checks fail sequentially then the status will be marked
// as bad.  Takes the same number of good checks to clear the status.
//
// Wraps another StatusMonitor to check the moment-in-time health status
type StatusMonitor struct {
	PollFrequency     time.Duration
	Timeout           time.Duration
	TriggerCount      int
	StatusChecker     StatusChecker
	stopChan          chan struct{}
	statusChan        chan bool
	slowCheckDuration time.Duration
}

var (
	timeoutFired chan struct{} // used only by unit tests
)

// goroutine that loops calling the StatusChecker
func (m *StatusMonitor) checker() {
	var (
		failCount    int // will not exceed TriggerCount
		healthy      = true
		healthReader <-chan error
		readTimeout  <-chan time.Time
		timer        = time.After(0) // fire immediately
		callStart    time.Time
	)
	for {
		select {
		case <-timer:
			// start a call to the StatusChecker in the background
			// so we can still service status requests ourselves
			// if the check blocks
			callStart = time.Now()
			healthReader = m.checkHealthy()
			readTimeout = time.After(m.Timeout)

		case err := <-healthReader:
			// StatusChecker returned a response
			if err != nil {
				failCount++
				if failCount > m.TriggerCount {
					failCount = m.TriggerCount
				} else if failCount == m.TriggerCount {
					// ohnoes
					healthy = false
					alog.Error{"action": "status_check", "status": "out_of_service",
						"fail_count": failCount, "error": err}.Log()
				}
			} else {
				if failCount > 0 {
					failCount--
					if failCount == 0 {
						// healthy again
						healthy = true
						alog.Info{"action": "status_check", "status": "return_to_service"}.Log()
					}
				}
			}
			timer = time.After(m.PollFrequency)
			readTimeout = nil
			duration := time.Now().Sub(callStart)
			if duration > m.slowCheckDuration {
				alog.Warn{"action": "status_check", "status": "slow_check", "duration": duration}.Log()
			}

		case <-readTimeout:
			// If the status check itself takes too long then mark the status
			// as immediately unhealthy.
			// still going to wait for the check to finish before starting another, though
			alog.Error{"action": "status_check", "status": "IsHealthy call taking too long!"}.Log()
			healthy = false
			failCount = m.TriggerCount
			if timeoutFired != nil {
				// unit by unit tests
				timeoutFired <- struct{}{}
			}

		case <-m.stopChan:
			close(m.statusChan)
			alog.Info{"action": "status_check", "status": "stopped"}.Log()
			return

		case m.statusChan <- healthy:
		}
	}
}

func (m *StatusMonitor) checkHealthy() chan error {
	result := make(chan error)
	go func() {
		result <- m.StatusChecker.IsHealthy()
	}()
	return result

}

// IsHealthy returns the current health status of the monitor
func (m *StatusMonitor) IsHealthy() error {
	if <-m.statusChan {
		return nil // healthy
	}
	return errors.New("Out of service")
}

// Stop the status checking goroutine.
//
// Any further called to IsHealthy will return an out of service error
// after the checker goroutine exits.
func (m *StatusMonitor) Stop() {
	close(m.stopChan)
}

// NewMonitor creates a StatusMonitor instance and starts the goroutine
// that polls the passed statusChecker according to the pollFrequency argument.
func NewMonitor(pollFrequency time.Duration, triggerCount int, statusChecker StatusChecker) *StatusMonitor {
	result := &StatusMonitor{
		PollFrequency:     pollFrequency,
		Timeout:           pollFrequency,
		TriggerCount:      triggerCount,
		StatusChecker:     statusChecker,
		stopChan:          make(chan struct{}),
		statusChan:        make(chan bool),
		slowCheckDuration: slowCheckDuration,
	}
	go result.checker()
	return result
}

// NewMonitorWithTimeout creates a StatusMonitor instance with separate timeout
// and poll frequencies, and starts the goroutine that polls the
// passed statusChecker according to the pollFrequency argument.
func NewMonitorWithTimeout(pollFrequency time.Duration, timeout time.Duration, triggerCount int, statusChecker StatusChecker) *StatusMonitor {
	result := &StatusMonitor{
		PollFrequency:     pollFrequency,
		Timeout:           timeout,
		TriggerCount:      triggerCount,
		StatusChecker:     statusChecker,
		stopChan:          make(chan struct{}),
		statusChan:        make(chan bool),
		slowCheckDuration: slowCheckDuration,
	}
	go result.checker()
	return result
}
