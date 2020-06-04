// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// The task package runs a function as a task
// It ensures only one instance of the task can be running at once
// and provides an HTTP interface to trigger the task and report on its status
package task

import (
	"encoding/json"
	"net/http"
	"net/url"
	"sync"
	"time"
)

// A task to run
type Task interface {
	Run(frm url.Values) error
}

type TaskFunc func(frm url.Values) error

func (f TaskFunc) Run(frm url.Values) error {
	return f(frm)
}

type Status string

const (
	Idle    Status = "idle"
	Running Status = "running"
	Failed  Status = "failed"
)

type Error struct {
	HttpStatus int
	Code       string
	Message    string
}

var (
	AlreadyRunning = &Error{
		HttpStatus: http.StatusConflict,
		Code:       "already_running",
		Message:    "Task already running",
	}
)

// A TaskRunner wraps a Task and executes it on demand, ensuring only
// one instance of the task is running concurrnetly.
type TaskRunner struct {
	sync.Mutex
	task         Task
	status       Status
	lastStarted  time.Time
	lastFinished time.Time
	lastError    error
}

func NewRunner(task Task) *TaskRunner {
	return &TaskRunner{
		task: task,
	}
}

func (t *TaskRunner) Run(frm url.Values) *Error {
	t.Lock()
	defer t.Unlock()
	if t.status == Running {
		return AlreadyRunning
	}
	t.status = Running
	t.lastStarted = time.Now()
	go t.runner(frm)
	return nil
}

func (t *TaskRunner) Status() Status {
	t.Lock()
	defer t.Unlock()
	return t.status
}

func (t *TaskRunner) LastStarted() time.Time {
	t.Lock()
	defer t.Unlock()
	return t.lastStarted
}

func (t *TaskRunner) LastFinished() time.Time {
	t.Lock()
	defer t.Unlock()
	return t.lastFinished
}

func (t *TaskRunner) LastError() error {
	t.Lock()
	defer t.Unlock()
	return t.lastError
}

func (t *TaskRunner) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	enc := json.NewEncoder(w)
	switch r.Method {
	case "GET":
		enc.Encode(t.getInfo())

	case "POST":
		r.ParseForm()
		if err := t.Run(r.PostForm); err != nil {
			w.WriteHeader(err.HttpStatus)
			enc.Encode(err)
		} else {
			enc.Encode(map[string]interface{}{"status": "Task started"})
		}

	default:
		w.WriteHeader(http.StatusBadRequest)
		enc.Encode(map[string]interface{}{"error": "Bad Method"})
	}
}

func (t *TaskRunner) getInfo() map[string]interface{} {
	var err string
	t.Lock()
	defer t.Unlock()
	if t.lastError != nil {
		err = t.lastError.Error()
	}
	return map[string]interface{}{
		"status":        t.status,
		"last_started":  t.lastStarted,
		"last_finished": t.lastFinished,
		"last_error":    err,
	}
}

func (t *TaskRunner) runner(frm url.Values) {
	err := t.task.Run(frm)
	t.Lock()
	defer t.Unlock()
	t.lastError = err
	t.lastFinished = time.Now()
	if err == nil {
		t.status = Idle
	} else {
		t.status = Failed
	}
}
