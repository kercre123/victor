package stats

import (
	"reflect"
	"testing"
)

func TestCounters(t *testing.T) {
	s := New()
	s.IncCounter("one", 1)
	s.IncCounter("two", 1)
	s.IncCounter("one", 1)

	counters, _ := s.Values(false)
	expected := map[string]Counter{
		"one": Counter{2, 2},
		"two": Counter{1, 1},
	}
	if !reflect.DeepEqual(counters, expected) {
		t.Errorf("expected=%#v actual=%#v", expected, counters)
	}

	s.ResetPeriod()
	counters, _ = s.Values(false)
	expected = map[string]Counter{
		"one": Counter{2, 0},
		"two": Counter{1, 0},
	}
	if !reflect.DeepEqual(counters, expected) {
		t.Errorf("expected=%#v actual=%#v", expected, counters)
	}
}

func TestActive(t *testing.T) {
	s := New()
	s.StartActive("one")
	s.StartActive("one")
	s.DoneActive("one")
	s.StartActive("two")
	s.StartActive("two")
	s.StartActive("three")

	_, active := s.Values(false)
	expected := map[string]Active{
		"one":   Active{1, 0, 2},
		"two":   Active{2, 0, 2},
		"three": Active{1, 0, 1},
	}

	if !reflect.DeepEqual(active, expected) {
		t.Errorf("expected=%#v actual=%#v", expected, active)
	}

	s.ResetPeriod() // should set Min and Max to current value
	s.DoneActive("two")
	s.StartActive("three")

	_, active = s.Values(false)
	expected = map[string]Active{
		"one":   Active{1, 1, 1},
		"two":   Active{1, 1, 2},
		"three": Active{2, 1, 2},
	}

	if !reflect.DeepEqual(active, expected) {
		t.Errorf("expected=%#v actual=%#v", expected, active)
	}
}

func TestValuesReset(t *testing.T) {
	s := New()
	s.StartActive("one")
	s.StartActive("one")
	s.DoneActive("one")
	s.IncCounter("two", 1)

	counters, active := s.Values(true)
	if counters["two"].Period != 1 || active["one"].Current != 1 {
		t.Fatal("Incorrect values")
	}

	counters, active = s.Values(true)
	if counters["two"].Period != 0 {
		t.Fatal("Values were not reset")
	}
	if counters["two"].AllTime != 1 {
		t.Fatal("Alltime value was reset")
	}

	if active["one"] != (Active{1, 1, 1}) {
		t.Fatal("Active value was not reset", active["one"])
	}
}

func TestAgg(t *testing.T) {
	s := New()
	vals := []int{5, 3, 4, 1, 5, 6, 5}
	for _, v := range vals {
		s.AddToAggregate("test", v)
	}

	r := s.aggregate["test"].Stats()
	expected := AggStats{
		Count:  7,
		Min:    1,
		Max:    6,
		Median: 5,
		Perc95: 6,
		Perc99: 6,
	}
	if r != expected {
		t.Errorf("Expected=%#v actual=%#v", expected, r)
	}
}

func TestLogValues(t *testing.T) {
	s := New()
	s.StartActive("one")
	s.StartActive("two")
	s.IncCounter("three", 1)

	s.ResetPeriod()
	s.IncCounter("three", 1)
	s.DoneActive("one")
	s.StartActive("two")

	clog, alog, _ := s.LogValues(true)
	clogExpected := map[string]interface{}{
		"three_alltime": int64(2),
		"three_period":  int64(1),
	}

	alogExpected := map[string]interface{}{
		"one_current":    int64(0),
		"one_period_min": int64(0),
		"one_period_max": int64(1),
		"two_current":    int64(2),
		"two_period_min": int64(1),
		"two_period_max": int64(2),
	}

	if !reflect.DeepEqual(clog, clogExpected) {
		t.Errorf("current vlaues mismatch expected=%#v actual=%#v", clogExpected, clog)
	}
	if !reflect.DeepEqual(alog, alogExpected) {
		t.Errorf("current vlaues mismatch expected=%#v actual=%#v", alogExpected, alog)
	}
}
