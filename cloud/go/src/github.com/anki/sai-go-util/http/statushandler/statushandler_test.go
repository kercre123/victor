package statushandler

import (
	"encoding/json"
	"errors"
	"fmt"
	"net/http"
	"net/http/httptest"
	"os"
	"strconv"
	"testing"
	"time"

	"github.com/anki/sai-go-util/buildinfo"
)

var infoTests = []struct {
	status         error
	expectedCode   int
	expectedStatus string
}{
	{nil, http.StatusOK, "ok"},
	{errors.New("fail"), http.StatusInternalServerError, "error"},
}

func TestStatusHandler(t *testing.T) {
	now := time.Now()
	buildinfo.ForceReset(buildinfo.BuildInfo{
		Product:       "product",
		BuildTime:     &now,
		BuildTimeUnix: strconv.FormatInt(now.Unix(), 10),
		Commit:        "commithash",
		Branch:        "branch",
		BuildHost:     "host",
		BuildUser:     "user",
	})

	hostname, _ := os.Hostname()
	expected := map[string]interface{}{
		"product":         "product",
		"build_time_unix": strconv.FormatInt(now.Unix(), 10),
		"build_time":      now.Format(time.RFC3339),
		"commit":          "commithash",
		"branch":          "branch",
		"build_host":      "host",
		"build_user":      "user",
		"hostname":        hostname,
	}

	for _, test := range infoTests {
		expected["status"] = test.expectedStatus
		r, _ := http.NewRequest("GET", "", nil)
		w := httptest.NewRecorder()
		handler := &StatusHandler{StatusCheckerFunc(func() error { return test.status })}
		handler.ServeHTTP(w, r)

		var result map[string]interface{}
		err := json.Unmarshal(w.Body.Bytes(), &result)
		if err != nil {
			t.Fatalf("Failed to unmarshal response for status=%v err=%s", test.status, err)
		}

		lastCheck, err := time.Parse(time.RFC3339, result["last_check"].(string))
		if err != nil {
			t.Errorf("Failed to parse last_check time time=%q", result["last_check"])
		} else if lastCheck.Before(now.Add(-time.Second)) { // now has fractional seconds
			t.Errorf("last check happened before we started?? time=%q parsed=%s now=%s", result["last_check"], lastCheck, now)
		}
		delete(result, "last_check")

		for k := range result {
			if result[k] != expected[k] {
				t.Errorf("Mismatch status=%v field=%s expected=%#v actual=%#v", test.status, k, expected[k], result[k])
			}
		}
		fmt.Println(len(result), len(expected))
		if w.Code != test.expectedCode {
			t.Errorf("code mismatch for status=%v expected=%d actual=%d", test.status, test.expectedCode, w.Code)
		}
	}
}

var errTest = errors.New("an error")
var healthSteps = []struct {
	checkerResponse error
	expectedHealthy bool
}{
	{nil, true},      // send a healthy response; should return healthy
	{errTest, true},  // send an error response; haven't crossed threshold of 2
	{errTest, false}, // send an error response again; meet threshold of 2
	{errTest, false}, // send an error response again; still unhealthy
	{errTest, false}, // send an error response again; still unhealthy
	{nil, false},     // send a healthy response; need one more to be healthy again
	{nil, true},      // all good again
}

func TestStatusMonitor(t *testing.T) {
	nextResponse := make(chan error)
	responder := StatusCheckerFunc(func() error {
		return <-nextResponse
	})

	monitor := NewMonitor(time.Duration(1*time.Millisecond), 2, responder)
	// should start in an unhealthy state
	if monitor.IsHealthy() != nil {
		t.Fatalf("IsHealthy returned unhealthy at startup")
	}

	for i, test := range healthSteps {
		// as the checker runs in a loop, we're guaranteed that the next read
		// using IsHealthy will return after it has evaluated a read from
		// our checker
		nextResponse <- test.checkerResponse
		healthy := monitor.IsHealthy() == nil
		if healthy != test.expectedHealthy {
			t.Errorf("Test step %d expected healthy=%t actual=%t", i+1, test.expectedHealthy, healthy)
		}
	}

	monitor.Stop()
	monitor.IsHealthy() // force the goroutine to cycle once more so this test doesn't race
	// should always be unhealthy now
	if monitor.IsHealthy() == nil {
		t.Error("Got a healthy resposne after stop")
	}

}

// Check that slow status functions cause the monitor to jump to a bad status state
func TestStatusMonitorTimeout(t *testing.T) {
	defer func(org time.Duration) { slowCheckDuration = org }(slowCheckDuration)
	slowCheckDuration = time.Microsecond

	nextResponse := make(chan error)
	responder := StatusCheckerFunc(func() error {
		return <-nextResponse
	})

	timeoutFired = make(chan struct{})
	monitor := NewMonitorWithTimeout(time.Duration(1*time.Second), time.Duration(1), 2, responder)
	defer func() {
		// ensure goroutine cleans up
		monitor.Stop()
		nextResponse <- nil
	}()

	select {
	case <-time.After(100 * time.Millisecond):
		t.Fatal("timeout didn't fire")
	case <-timeoutFired:
	}
	if monitor.IsHealthy() == nil {
		t.Error("monitor still reports healthy")
	}

}
