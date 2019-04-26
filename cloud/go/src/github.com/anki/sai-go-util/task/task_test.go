package task

import (
	"encoding/json"
	"errors"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"net/url"
	"reflect"
	"strings"
	"testing"
	"time"

	"github.com/kr/pretty"
)

func TestRunnerSingleTask(t *testing.T) {
	// Check that a runner will not allow more than one instance of a task
	// to run concurrently
	release := make(chan error)
	task := TaskFunc(func(frm url.Values) error { return <-release })
	runner := NewRunner(task)

	if err := runner.Run(nil); err != nil {
		t.Fatal("Unexpected error on inital run", err)
	}

	// attempt to start another; should return an error
	if err := runner.Run(nil); err != AlreadyRunning {
		t.Fatal("Did not get AlreadyRunning error; got ", err)
	}

	// complete the task, should now be able to start another task
	release <- nil
	if err := runner.Run(nil); err != nil {
		t.Fatal("Unexpected error on third run", err)
	}
}

func TestRunnerStatus(t *testing.T) {
	// hit all the status endpoints.. most useful when tested with -race
	now := time.Now().Add(-time.Second)
	release := make(chan error)
	task := TaskFunc(func(frm url.Values) error { return <-release })
	runner := NewRunner(task)

	if err := runner.Run(nil); err != nil {
		t.Fatal("Unexpected error on inital run", err)
	}

	generatedError := errors.New("Failed")
	release <- generatedError

	if status := runner.Status(); status != Failed {
		t.Error("Incorrect status", status)
	}

	if runner.LastStarted().Before(now) {
		t.Error("Incorrect LastStarted time", runner.LastStarted())
	}

	if runner.LastFinished().Before(now) {
		t.Error("Incorrect LastFinished time", runner.LastFinished())
	}

	if err := runner.LastError(); err != generatedError {
		t.Error("Incorrect LastError", err)
	}
}

func TestRunnerHttpGet(t *testing.T) {
	// Check that a get request returns correct status information
	release := make(chan error)
	task := TaskFunc(func(frm url.Values) error { return <-release })
	runner := NewRunner(task)
	runner.Run(nil)
	release <- errors.New("Test error")

	recorder := httptest.NewRecorder()
	request, _ := http.NewRequest("GET", "", nil)

	runner.ServeHTTP(recorder, request)

	if recorder.Code != 200 {
		t.Fatal("Did not get 200 response", recorder.Code)
	}

	var result map[string]interface{}
	if err := json.NewDecoder(recorder.Body).Decode(&result); err != nil {
		t.Fatal("Failed to decode HTTP body err=", err)
	}

	expected := map[string]interface{}{
		"status":        "failed",
		"last_started":  runner.LastStarted().Format(time.RFC3339Nano),
		"last_finished": runner.LastFinished().Format(time.RFC3339Nano),
		"last_error":    "Test error",
	}
	if !reflect.DeepEqual(expected, result) {
		t.Errorf("Did not get expected result.\nExpected=%#v\nactual=%#v\ndiff=%#v",
			pretty.Formatter(expected), pretty.Formatter(result), pretty.Diff(expected, result))
	}
}

func TestRunnerHttpPostOk(t *testing.T) {
	// Check that a get request returns correct status information
	release := make(chan error)
	task := TaskFunc(func(frm url.Values) error { return <-release })
	runner := NewRunner(task)

	recorder := httptest.NewRecorder()
	request, _ := http.NewRequest("POST", "", nil)

	runner.ServeHTTP(recorder, request)

	if recorder.Code != 200 {
		t.Fatal("Did not get 200 response", recorder.Code)
	}

	if status := runner.Status(); status != Running {
		t.Errorf("Task wasn't started - status=%s", status)
	}

	// ensure we release the goroutine
	release <- errors.New("Test error")
}

func TestRunnerHttpPostWithForm(t *testing.T) {
	// Check that a post request passes through form values
	release := make(chan error)
	var vals url.Values
	task := TaskFunc(func(frm url.Values) error { vals = frm; return <-release })
	runner := NewRunner(task)

	recorder := httptest.NewRecorder()
	request, _ := http.NewRequest("POST", "", nil)
	request.Header.Set("Content-Type", "application/x-www-form-urlencoded")
	request.Body = ioutil.NopCloser(
		strings.NewReader(
			url.Values{"frmkey1": {"frmval1"}}.Encode()))
	runner.ServeHTTP(recorder, request)

	if recorder.Code != 200 {
		t.Fatal("Did not get 200 response", recorder.Code)
	}

	if status := runner.Status(); status != Running {
		t.Errorf("Task wasn't started - status=%s", status)
	}

	release <- errors.New("Test error")

	if vals.Get("frmkey1") != "frmval1" {
		t.Errorf("Form was not passed through correctly.  Got %#v", vals)
	}
}

func TestRunnerHttpPostFail(t *testing.T) {
	// Check that a get request returns correct status information
	release := make(chan error)
	task := TaskFunc(func(frm url.Values) error { return <-release })
	runner := NewRunner(task)
	// start the job so the POST will fail with a task running error
	runner.Run(nil)

	// ensure we release the goroutine
	defer func() { release <- errors.New("Test error") }()

	recorder := httptest.NewRecorder()
	request, _ := http.NewRequest("POST", "", nil)

	runner.ServeHTTP(recorder, request)

	if recorder.Code != http.StatusConflict {
		t.Fatalf("Unexpected http response expected=%d actual=%d",
			http.StatusConflict, recorder.Code)
	}

	if status := runner.Status(); status != Running {
		t.Errorf("Task wasn't started - status=%s", status)
	}
}

func TestRunnerHttpPostBadMethod(t *testing.T) {
	task := TaskFunc(func(frm url.Values) error { return nil })
	runner := NewRunner(task)

	recorder := httptest.NewRecorder()
	request, _ := http.NewRequest("DELETE", "", nil)

	runner.ServeHTTP(recorder, request)

	if recorder.Code != http.StatusBadRequest {
		t.Fatalf("Unexpected http response expected=%d actual=%d",
			http.StatusConflict, recorder.Code)
	}
}
