// Copyright 2016 Anki, Inc.

package logsender

import (
	"math"
	"sync"
)

// TODO: switch times to time.Duration
const maxFlushTimes = 60

// Stats hold current Sender statistics.
type Stats struct {
	EntryCount            int64
	FlushCount            int64
	FlushFullCount        int64
	ThruputExceededErrors int64
	OtherPutErrors        int64
	FlushTimes            [maxFlushTimes]int64
	HardErrors            int64
	DroppedWrites         int64
	nextFlushTimeIdx      uint8
}

// FlushTimeStats returns the min, max and average time taken to flush
// data to Kinesis using a put call, in milliseconds.
func (s *Stats) FlushTimeStats() (min, max int64, avg float64) {
	var sum, count int64
	min = math.MaxInt64
	for _, v := range s.FlushTimes {
		if v < 1 {
			continue
		}
		if v < min {
			min = v
		}
		if v > max {
			max = v
		}
		sum += v
		count++
	}

	return min, max, (float64(sum) / float64(count))
}

type stats struct {
	m sync.Mutex
	Stats
}

func (s *stats) incValue(v *int64) {
	s.m.Lock()
	defer s.m.Unlock()
	*v++
}

func (s *stats) addFlushTime(t int64) {
	s.m.Lock()
	defer s.m.Unlock()
	s.FlushTimes[s.nextFlushTimeIdx%maxFlushTimes] = t
	s.nextFlushTimeIdx++
}

func (s *stats) values() Stats {
	s.m.Lock()
	defer s.m.Unlock()
	return s.Stats
}
