package metricshandler

import (
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/aalpern/go-metrics"
	"github.com/anki/sai-go-util/metricstesting"
)

var methodCountTests = []struct {
	method string
	path   string
	count  int
}{
	{"GET", "/", 12},
	{"PUT", "/thing", 37},
	{"POST", "/thing", 22},
	{"HEAD", "/funny.gif", 189},
	{"DELETE", "/wat", 2},
	{"OPTIONS", "/", 7},
}

func TestMethodCounts(t *testing.T) {
	for _, test := range methodCountTests {
		registry := metrics.NewRegistry()
		h := HandlerWithRegistry("/", http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {}),
			registry)
		r, _ := http.NewRequest(test.method, test.path, nil)
		w := httptest.NewRecorder()
		for i := 0; i < test.count; i++ {
			h.ServeHTTP(w, r)
		}
		metricstesting.AssertTimerCount(t, registry, "all."+test.method, test.count)
		metricstesting.AssertTimerCount(t, registry, "path."+"/"+"."+test.method, test.count)
	}
}

var responseCodeTests = []struct {
	metric string
	code   int
	count  int
}{
	{"response.1xx", 100, 3},
	{"response.2xx", 200, 7},
	{"response.3xx", 306, 9},
	{"response.4xx", 404, 22},
	{"response.5xx", 503, 6},
}

func TestResponseCountCounts(t *testing.T) {
	registry := metrics.NewRegistry()
	for _, test := range responseCodeTests {
		h := HandlerWithRegistry("/", http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(test.code)
		}), registry)
		r, _ := http.NewRequest("GET", "/", nil)
		w := httptest.NewRecorder()
		for i := 0; i < test.count; i++ {
			h.ServeHTTP(w, r)
		}
		metricstesting.AssertCounterCount(t, registry, test.metric, test.count)
	}
}
