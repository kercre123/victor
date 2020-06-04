package ratelimit

import (
	"net/http"
	"net/http/httptest"
	"reflect"
	"sort"
	"testing"
	"time"
)

func TestLimiter(t *testing.T) {
	// Setup a limiter with a 2 in 100ms limit
	var callcount int
	handler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		callcount++
	})

	req, _ := http.NewRequest("GET", "/", nil)
	req.RemoteAddr = "1.2.3.4"
	limit := NewIPLimiter(handler, 0.1, 2, time.Minute, nil)
	defer limit.Close()

	// first attempt should succeed
	recorder := httptest.NewRecorder()
	limit.ServeHTTP(recorder, req)
	if callcount != 1 || recorder.Code != 200 {
		t.Errorf("Attempt 1 failed callcount=%d code=%d", callcount, recorder.Code)
	}

	// second attempt should also succeed
	recorder = httptest.NewRecorder()
	limit.ServeHTTP(recorder, req)
	if callcount != 2 || recorder.Code != 200 {
		t.Errorf("Attempt 2 failed callcount=%d code=%d", callcount, recorder.Code)
	}

	// third attempt should be limited
	recorder = httptest.NewRecorder()
	limit.ServeHTTP(recorder, req)
	if callcount != 2 || recorder.Code != 429 {
		t.Errorf("Attempt 3 succeeded when should have failed callcount=%d code=%d", callcount, recorder.Code)
	}

	// request from a differnet IP should still suceed
	req.RemoteAddr = "1.2.3.5"
	recorder = httptest.NewRecorder()
	limit.ServeHTTP(recorder, req)
	if callcount != 3 || recorder.Code != 200 {
		t.Errorf("Attempt 4 failed callcount=%d code=%d", callcount, recorder.Code)
	}
}

func TestMaxRefillTime(t *testing.T) {
	limit := &RateLimiter{
		// 2 second limit
		rate:     10, // 10 tokens/second
		capacity: 20, // 20 token capacity
	}
	rt := limit.maxRefillTime()
	if rt != 2*time.Second {
		t.Errorf("Expected 2 seconds, got %s", rt)
	}
}

func TestSweeper(t *testing.T) {
	now := time.Now()
	limit := &RateLimiter{
		shutdown: make(chan struct{}),
		// 2 second limit
		rate:           10,
		capacity:       20,
		sweepFrequency: 10 * time.Millisecond,
		buckets: map[interface{}]*bucket{
			"1.0.0.1": &bucket{lastSeen: now.Add(-3 * time.Second)}, // expired
			"1.0.0.2": &bucket{lastSeen: now.Add(-5 * time.Second)}, // expired
			"1.0.0.3": &bucket{lastSeen: now.Add(-1 * time.Second)},
			"1.0.0.4": &bucket{lastSeen: now.Add(-100 * time.Millisecond)},
		},
	}

	// let the sweeper run once
	go limit.sweeper()
	time.Sleep(15 * time.Millisecond)
	limit.shutdown <- struct{}{}

	var keysleft []string
	for k, _ := range limit.buckets {
		keysleft = append(keysleft, k.(string))
	}

	expected := []string{"1.0.0.3", "1.0.0.4"}
	sort.Strings(keysleft)

	if !reflect.DeepEqual(expected, keysleft) {
		t.Errorf("Expected buckets were not removed.  expected remaining=%#v actual=%#v", expected, keysleft)
	}
}

var nilHandler = http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {})

func TestWhitelist(t *testing.T) {
	// immediately exceeded rate limit
	limit := NewIPLimiter(nilHandler, 0.01, 1, 0, nil)

	req, _ := http.NewRequest("GET", "/", nil)
	req.RemoteAddr = "1.1.1.1"

	// exceed the limit with 1 request
	recorder := httptest.NewRecorder()
	limit.ServeHTTP(recorder, req)

	// No whitelist set; should always fail
	recorder = httptest.NewRecorder()
	limit.ServeHTTP(recorder, req)

	if recorder.Code != 429 {
		t.Errorf("Did not fail with no whitelist code=%d", recorder.Code)
	}

	// Add a whitelist based on a header
	limit.whitelistByHeader = map[string][]string{
		"My-Header": {"one", "two"},
	}

	// Request with a header that matches
	req.Header.Set("My-header", "two")
	recorder = httptest.NewRecorder()
	limit.ServeHTTP(recorder, req)

	if recorder.Code != 200 {
		t.Errorf("Did not succeed with valid header code=%d", recorder.Code)
	}

	// Request with a header that does not match
	req.Header.Set("My-header", "nomatch")
	recorder = httptest.NewRecorder()
	limit.ServeHTTP(recorder, req)
	if recorder.Code != 429 {
		t.Errorf("Did not fail with bad header code=%d", recorder.Code)
	}

	// Request with no header at all
	req, _ = http.NewRequest("GET", "/", nil)
	req.RemoteAddr = "1.1.1.1"
	recorder = httptest.NewRecorder()
	limit.ServeHTTP(recorder, req)
	if recorder.Code != 429 {
		t.Errorf("Did not fail with no header code=%d", recorder.Code)
	}
}

type testBucketer struct {
	count int
}

func (tb *testBucketer) BucketId(w http.ResponseWriter, r *http.Request) interface{} {
	return "foo"
}

func (tb *testBucketer) RequestLimited(r *http.Request, bucketId interface{}, exceedCount int) {
	tb.count = exceedCount
}

func TestReporter(t *testing.T) {
	tb := &testBucketer{}
	l := NewLimiter(tb, nilHandler, 1, 1, time.Minute, nil)

	req, _ := http.NewRequest("GET", "/", nil)
	recorder := httptest.NewRecorder()
	l.ServeHTTP(recorder, req)

	if recorder.Code != 200 {
		t.Fatal("Did not get 200")
	}
	if tb.count != 0 {
		t.Fatal("count != 0", tb.count)
	}

	l.ServeHTTP(recorder, req)
	if recorder.Code != 429 {
		t.Fatal("Did not get 429 (1)")
	}
	if tb.count != 1 {
		t.Fatal("count != 1", tb.count)
	}

	l.ServeHTTP(recorder, req)
	if recorder.Code != 429 {
		t.Fatal("Did not get 429 (2)")
	}
	if tb.count != 2 {
		t.Fatal("count != 2", tb.count)
	}
}
