// metricstesting provides simple assertion utilities for testing
// metrics-instrumented code. They can be used to ensure that code is
// properly instrument, or to capitalize on that instrumentation to
// ensure that expected function calls are being made during unit
// tests.
package metricstesting

import (
	"testing"

	"github.com/aalpern/go-metrics"
)

// AssertTimerCount asserts that a specific timer metrics has the
// specified value, producing an error if it does not.
func AssertTimerCount(t *testing.T, registry metrics.Registry, name string, count int) {
	m := registry.Get(name)
	if m == nil {
		t.Errorf("Missing metrics %s", name)
	}
	c := m.(metrics.Timer)
	if c == nil {
		t.Errorf("Metric %s is not a timer. actual: %v", name, m)
	}
	if c.Count() != int64(count) {
		t.Errorf("Metric %s has incorrect value. Expected %d, was %d", name, count, c.Count())
	}
}

// AssertCounterCount asserts that a specific counter metrics has the
// specified value, producing an error if it does not.
func AssertCounterCount(t *testing.T, registry metrics.Registry, name string, count int) {
	m := registry.Get(name)
	if m == nil {
		t.Errorf("Missing metrics %s", name)
	}
	c := m.(metrics.Counter)
	if c == nil {
		t.Errorf("Metric %s is not a counter. actual: %v", name, m)
	}
	if c.Count() != int64(count) {
		t.Errorf("Metric %s has incorrect value. Expected %d, was %d", name, count, c.Count())
	}
}

// Countable describes the subset of both Counter and Meter metrics
// that returns a count (since all meters also track counts). You'd
// think that Meter would be a superset of Counter, but it's not...
type Countable interface {
	Count() int64
}

// AssertCount asserts that a specific counter metric has the
// specified value, producing an error if it does not. It does not
// matter if the underlying metric is a Timer, a Counter, or a Meter,
// as long as it has a Count() method.
func AssertCount(t *testing.T, registry metrics.Registry, name string, count int64) {
	m := registry.Get(name)
	if m == nil {
		// This should not be an error unless the expected value is
		// non-zero -- metrics can be lazily allocated, and may not
		// exist until they're incremented above zero.
		if count > 0 {
			t.Errorf("Metric %s had expected value of %d, but was not found", name, count)
		}
		return
	}
	if c, ok := m.(Countable); ok {
		if c.Count() != count {
			t.Errorf("Metric %s has incorrect value. Expected %d, was %d", name, count, c.Count())
		}
	} else {
		t.Errorf("Metric %s is not a countable type. actual: %v", name, m)
	}
}

// AssertCounts takes a map of metric names to values, to assert
// expected values for a group of metrics in one go.
func AssertCounts(t *testing.T, registry metrics.Registry, data map[string]int64) {
	for k, v := range data {
		AssertCount(t, registry, k, v)
	}
}

func GetCount(registry metrics.Registry, name string) int64 {
	m := registry.Get(name)
	if m == nil {
		return 0
	}
	if c, ok := m.(Countable); !ok {
		return -1
	} else {
		return c.Count()
	}
}
