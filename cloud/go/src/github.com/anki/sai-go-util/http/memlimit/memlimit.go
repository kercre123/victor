// memlimit provides a simple HTTP handler that restricts access to the server
// when too much memory is currently in use.
package memlimit

import (
	"net/http"
	"runtime"
)

// MemLimiter wraps a standard HTTP handler and returns a 503 Server Too Busy response
// if the current memory usage exceeds the configured maximum.
type MemLimiter struct {
	handler      http.Handler
	maxHeapInuse uint64
	onBusy       func()
}

// NewMemLimiter returns an initialized MemLimiter.
// onBusy is an optional function that should be called whenever the handler returns
// a 503 resposne due to the current memory usage being too high.
func NewMemLimiter(handler http.Handler, maxHeapInUse uint64, onBusy func()) *MemLimiter {
	return &MemLimiter{handler, maxHeapInUse, onBusy}
}

func (ml *MemLimiter) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	m := &runtime.MemStats{}
	runtime.ReadMemStats(m)
	if m.HeapInuse > ml.maxHeapInuse {
		http.Error(w, "Server Too Busy", http.StatusServiceUnavailable)
		if ml.onBusy != nil {
			ml.onBusy()
		}
		return
	}
	ml.handler.ServeHTTP(w, r)
}
