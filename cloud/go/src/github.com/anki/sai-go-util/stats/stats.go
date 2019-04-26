// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// The stats package provides a simple mechanism for tracking periodic
// atomic statistics information.
package stats

import (
	"math"
	"sort"
	"sync"
)

type Counter struct {
	AllTime int64
	Period  int64
}

type Active struct {
	Current   int64
	PeriodMin int64
	PeriodMax int64
}

type Aggregate []int

type AggStats struct {
	Count  int
	Median float64
	Max    int
	Min    int
	Perc95 int
	Perc99 int
}

func (a Aggregate) Stats() (result AggStats) {
	if len(a) == 0 {
		return result
	}
	sort.Ints(a)
	return AggStats{
		Count:  len(a),
		Median: a.median(),
		Max:    a[len(a)-1],
		Min:    a[0],
		Perc95: a.perc(95),
		Perc99: a.perc(99),
	}
}

func (a Aggregate) median() float64 {
	if len(a) == 0 {
		return 0
	}
	if len(a) == 1 {
		return float64(a[0])
	}
	h := len(a) / 2
	if len(a)%2 == 0 {
		return float64(a[h-1]+a[h]) / 2
	}
	return float64(a[h])
}

func (a Aggregate) perc(pct int) int {
	p := float64(pct) / 100
	o := int(math.Floor(float64(len(a)) * p))
	return a[o]
}

// Stats holds a set of named cumulative counters, which consist of an "all-time"
// and period component.  The period component may be reset to zero at any time.
//
// It also holds a set of active counters which maybe used to track concurrent
// activity by calling StartActive and DoneActive.
type Stats struct {
	cm       sync.Mutex
	counters map[string]*Counter

	am     sync.Mutex
	active map[string]*Active

	aggm      sync.Mutex
	aggregate map[string]Aggregate
}

// New returns a new thread-safe set of stats counters.
func New() *Stats {
	return &Stats{
		counters:  make(map[string]*Counter),
		active:    make(map[string]*Active),
		aggregate: make(map[string]Aggregate),
	}
}

// IncCounter increments the named counter by one for both
// it's all-time and period value.
func (s *Stats) IncCounter(name string, qty int64) {
	s.cm.Lock()
	defer s.cm.Unlock()
	v := s.counters[name]
	if v == nil {
		v = &Counter{}
		s.counters[name] = v
	}
	v.AllTime += qty
	v.Period += qty
}

func (s *Stats) AddToAggregate(name string, v int) {
	s.aggm.Lock()
	defer s.aggm.Unlock()
	s.aggregate[name] = append(s.aggregate[name], v)
}

// StartActive inrements a named activity counter by one.
// Call DoneActive to decrement.
func (s *Stats) StartActive(name string) {
	s.am.Lock()
	defer s.am.Unlock()
	v := s.active[name]
	if v == nil {
		v = &Active{}
		s.active[name] = v
	}
	v.Current++
	if v.Current > v.PeriodMax {
		v.PeriodMax = v.Current
	}
}

// DoneActive decrements the named activity counter by one.
func (s *Stats) DoneActive(name string) {
	s.am.Lock()
	defer s.am.Unlock()
	v := s.active[name]
	if v == nil {
		v = &Active{}
		s.active[name] = v
	}
	v.Current--
	if v.Current < v.PeriodMin {
		v.PeriodMin = v.Current
	}
}

func (s *Stats) resetPeriod() {
	for _, v := range s.counters {
		v.Period = 0
	}
	for _, v := range s.active {
		v.PeriodMax = v.Current
		v.PeriodMin = v.Current
	}
	for k, v := range s.aggregate {
		s.aggregate[k] = v[0:0]
	}
}

func (s *Stats) ResetPeriod() {
	s.cm.Lock()
	s.am.Lock()
	defer s.cm.Unlock()
	defer s.am.Unlock()
	s.resetPeriod()
}

func (s *Stats) Values(resetPeriod bool) (counters map[string]Counter, active map[string]Active) {
	s.cm.Lock()
	s.am.Lock()
	s.aggm.Lock()
	defer s.cm.Unlock()
	defer s.am.Unlock()
	defer s.aggm.Unlock()

	counters = make(map[string]Counter)
	active = make(map[string]Active)

	for k, v := range s.counters {
		counters[k] = *v
	}
	for k, v := range s.active {
		active[k] = *v
	}
	if resetPeriod {
		s.resetPeriod()
	}
	return counters, active
}

// LogValues generates log entries for all entries
func (s *Stats) LogValues(resetPeriod bool) (countLog, activeLog, aggLog map[string]interface{}) {
	s.cm.Lock()
	s.am.Lock()
	s.aggm.Lock()
	defer s.cm.Unlock()
	defer s.am.Unlock()
	defer s.aggm.Unlock()

	countLog = make(map[string]interface{})
	for k, v := range s.counters {
		countLog[k+"_alltime"] = v.AllTime
		countLog[k+"_period"] = v.Period
	}

	activeLog = make(map[string]interface{})
	for k, v := range s.active {
		activeLog[k+"_current"] = v.Current
		activeLog[k+"_period_min"] = v.PeriodMin
		activeLog[k+"_period_max"] = v.PeriodMax
	}

	aggLog = make(map[string]interface{})
	for k, v := range s.aggregate {
		s := v.Stats()
		aggLog[k+"_count"] = s.Count
		aggLog[k+"_median"] = s.Median
		aggLog[k+"_min"] = s.Min
		aggLog[k+"_max"] = s.Max
		aggLog[k+"_perc95"] = s.Perc95
		aggLog[k+"_perc99"] = s.Perc99
	}

	if resetPeriod {
		s.resetPeriod()
	}

	return countLog, activeLog, aggLog
}
