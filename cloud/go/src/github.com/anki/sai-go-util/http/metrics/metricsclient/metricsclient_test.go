package metricsclient

import (
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/aalpern/go-metrics"
	"github.com/anki/sai-go-util/metricstesting"
)

var methodCountTests = []struct {
	method string
	count  int
}{
	{"GET", 5},
	{"PUT", 6},
	{"POST", 6},
	{"OPTIONS", 8},
	{"HEAD", 9},
	{"DELETE", 10},
}

func TestMethodCountsGeneric(t *testing.T) {
	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {}))
	defer ts.Close()

	total := 0
	client, err := New("test")
	if err != nil {
		t.Fatalf("%s", err)
	}

	for _, test := range methodCountTests {
		total += test.count
		for i := 0; i < test.count; i++ {
			req, _ := http.NewRequest(test.method, ts.URL, nil)
			client.Do(req)
		}
		metricstesting.AssertTimerCount(t, metrics.DefaultRegistry,
			"http.client.test.method."+test.method,
			test.count)
	}
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry,
		"http.client.test.method.ALL", total)
}

func TestMethodCountsGet(t *testing.T) {
	client, err := New("test2")
	if err != nil {
		t.Fatalf("%s", err)
	}
	count := 4

	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {}))
	defer ts.Close()
	for i := 0; i < count; i++ {
		client.Get(ts.URL)
	}
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test2.method.ALL", count)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test2.method.GET", count)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test2.method.PUT", 0)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test2.method.POST", 0)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test2.method.HEAD", 0)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test2.method.DELETE", 0)
}

func TestMethodCountsHead(t *testing.T) {
	client, err := New("test3")
	if err != nil {
		t.Fatalf("%s", err)
	}
	count := 5

	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {}))
	defer ts.Close()
	for i := 0; i < count; i++ {
		client.Head(ts.URL)
	}
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test3.method.ALL", count)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test3.method.GET", 0)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test3.method.PUT", 0)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test3.method.POST", 0)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test3.method.HEAD", count)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test3.method.DELETE", 0)
}

func TestMethodCountsPost(t *testing.T) {
	client, err := New("test4")
	if err != nil {
		t.Fatalf("%s", err)
	}
	count := 6

	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {}))
	defer ts.Close()
	for i := 0; i < count; i++ {
		client.Post(ts.URL, "text/plain", nil)
	}
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test4.method.ALL", count)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test4.method.GET", 0)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test4.method.PUT", 0)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test4.method.POST", count)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test4.method.HEAD", 0)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test4.method.DELETE", 0)
}

func TestMethodCountsPostForm(t *testing.T) {
	client, err := New("test5")
	if err != nil {
		t.Fatalf("%s", err)
	}
	count := 7

	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {}))
	defer ts.Close()
	for i := 0; i < count; i++ {
		client.PostForm(ts.URL, nil)
	}
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test5.method.ALL", count)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test5.method.GET", 0)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test5.method.PUT", 0)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test5.method.POST", count)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test5.method.HEAD", 0)
	metricstesting.AssertTimerCount(t, metrics.DefaultRegistry, "http.client.test5.method.DELETE", 0)
}

var responseCountTests = []struct {
	metric string
	code   int
	count  int
}{
	// Use 101 here, not 100 - 100 Continue means the server is
	// telling the client it can continue to send more data.
	{"response.1xx", 101, 1},
	{"response.2xx", 201, 2},
	{"response.3xx", 300, 3},
	{"response.4xx", 401, 4},
	{"response.5xx", 503, 5},
}

func TestResponseCounts(t *testing.T) {
	client, err := New("responses")
	if err != nil {
		t.Fatalf("%s", err)
	}

	for _, test := range responseCountTests {
		ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(test.code)
		}))
		for i := 0; i < test.count; i++ {
			client.Get(ts.URL)
		}
		ts.CloseClientConnections()
		ts.Close()
		metricstesting.AssertCounterCount(t, metrics.DefaultRegistry, "http.client.responses."+test.metric, test.count)
	}
}

func TestNilTransportError(t *testing.T) {
	_, err := New("whatever", WithTransport(nil))
	if err == nil {
		t.Fatalf("Failed to prevent nil transport")
	}
}

type myTransport struct{}

func (t myTransport) RoundTrip(req *http.Request) (*http.Response, error) {
	return nil, nil
}

func TestWithTransport(t *testing.T) {
	client, err := New("whatever", WithTransport(&myTransport{}))
	if err != nil {
		t.Fatalf("%s", err)
	}
	if mt, ok := client.Transport.(*metricsTransport); !ok {
		t.Errorf("Failed to set metrics transport correctly")
	} else {
		if _, ok = mt.transport.(*myTransport); !ok {
			t.Errorf("Failed to set wrapped transport correctly")
		}
	}
}
