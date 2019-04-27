// Copyright 2016 Anki, Inc.

package logsender

import (
	"log"
	"sync"
	"time"
)

// Monitor periodically collects Sender statistics and sends them to
// a log.Logger.
type Monitor struct {
	// Sender specifies which sender should be monitored - Required.
	Sender *Sender

	// Logger specifies where the log entry should be sent to - Required.
	Logger *log.Logger

	// LogInterval specifies how often a log entry should be generated.
	// Defaults to once a minute.
	LogInterval time.Duration

	stop chan chan struct{}
	once sync.Once
}

// Start starts a goroutine generating log events firing every LogInterval.
func (m *Monitor) Start() {
	if m.LogInterval == 0 {
		m.LogInterval = time.Minute
	}
	if m.Logger == nil {
		panic("Logger not set")
	}
	if m.Sender == nil {
		panic("Sender not set")
	}

	m.stop = make(chan chan struct{})

	go func() {
		for {
			select {
			case <-time.After(m.LogInterval):
				stats := m.Sender.Stats()
				min, max, avg := stats.FlushTimeStats()
				m.Logger.Printf("dropped_writes=%d entry_count=%d flush_count=%d "+
					"hard_errors=%d other_put_errors=%d thruput_exceeeded_errors=%d "+
					"put_time_min=%d put_time_max=%d put_time_avg=%.1f",
					stats.DroppedWrites, stats.EntryCount, stats.FlushCount,
					stats.HardErrors, stats.OtherPutErrors, stats.ThruputExceededErrors,
					min, max, avg)
			case ch := <-m.stop:
				ch <- struct{}{}
				return
			}
		}
	}()
}

// Stop stops the monitor.  No more writes to the logger will occur after
// this method returns.
func (m *Monitor) Stop() {
	m.once.Do(func() {
		ch := make(chan struct{})
		m.stop <- ch
		<-ch
		close(m.stop)
	})
}
