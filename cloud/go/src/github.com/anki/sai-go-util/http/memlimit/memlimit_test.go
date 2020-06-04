package memlimit

import (
	"net/http"
	"net/http/httptest"
	"runtime"
	"testing"
)

type testHandler struct {
	callCount int
}

func (h *testHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	h.callCount++
}

func TestLimiter(t *testing.T) {
	runtime.GC()
	m := &runtime.MemStats{}
	runtime.ReadMemStats(m)
	n := m.HeapInuse + 100000
	h := &testHandler{}
	limiter := NewMemLimiter(h, n, nil)

	// should not be limited
	r, _ := http.NewRequest("GET", "/", nil)
	w := httptest.NewRecorder()
	limiter.ServeHTTP(w, r)
	if h.callCount != 1 {
		t.Fatalf("Handler was not called.  Response=%v", w)
	}

	// alloc some memory; not sure how reliable this will be as the optimizer improves
	foo := make([]byte, 1000000)
	foo[100] = 'x'

	limiter.ServeHTTP(w, r)
	if h.callCount != 1 {
		t.Fatalf("Handler was called.  Response=%v", w)
	}

	if w.Code != 503 {
		t.Errorf("Handler did not set 503.  Response=%v", w)
	}
}

func TestOnBusy(t *testing.T) {
	var obCalled bool

	runtime.GC()
	ob := func() { obCalled = true }

	m := &runtime.MemStats{}
	runtime.ReadMemStats(m)
	n := m.HeapInuse + 100000
	h := &testHandler{}
	limiter := NewMemLimiter(h, n, ob)

	// should not be limited
	r, _ := http.NewRequest("GET", "/", nil)
	w := httptest.NewRecorder()
	limiter.ServeHTTP(w, r)
	if h.callCount != 1 {
		t.Fatalf("Handler was not called.  Response=%v", w)
	}

	if obCalled {
		t.Fatal("OnBusy was called for a non busy request")
	}

	// alloc some memory; not sure how reliable this will be as the optimizer improves
	foo := make([]byte, 1000000)
	foo[100] = 'x'

	limiter.ServeHTTP(w, r)
	if h.callCount != 1 {
		t.Fatalf("Handler was called.  Response=%v", w)
	}

	if w.Code != 503 {
		t.Errorf("Handler did not set 503.  Response=%v", w)
	}

	if !obCalled {
		t.Fatal("OnBusy was not called for a non busy request")
	}
}
