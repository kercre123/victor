package svc

import (
	"context"
	"os"
	"os/signal"
	"path/filepath"
	"runtime"
	"runtime/pprof"
	"sync"
	"syscall"
	"time"

	"github.com/anki/sai-go-util/log"
)

// ----------------------------------------------------------------------
// Service Components
// ----------------------------------------------------------------------

// WithRuntimeLogger configures a composite service object with a
// component that launches a go routine to log Go runtime memory stats
// once a minute.
func WithRuntimeLogger() ConfigFn {
	var wg sync.WaitGroup
	stop := make(chan struct{})

	return WithComponent(&SimpleServiceObject{
		OnStop: func() error {
			close(stop)
			wg.Wait()
			return nil
		},
		OnKill: func() error {
			close(stop)
			return nil
		},
		OnStart: func(ctx context.Context) error {
			wg.Add(1)
			go func() {
				for {
					var gcAlltime uint64
					m := new(runtime.MemStats)
					runtime.ReadMemStats(m)
					var total256Pause, avg256Pause, count uint64
					for count = 0; count < 256; count++ {
						if m.PauseNs[count] == 0 {
							break
						}
						total256Pause += m.PauseNs[count]
					}
					if count > 0 {
						avg256Pause = total256Pause / count
					}
					if ngc := uint64(m.NumGC); ngc > 0 {
						gcAlltime = m.PauseTotalNs / ngc
					}
					alog.Info{
						"action":            "runtime_stats",
						"goroutines":        runtime.NumGoroutine(),
						"mem_in_use":        m.Alloc,
						"mem_allocated":     m.TotalAlloc,
						"mem_system":        m.Sys,
						"heap_alloc":        m.HeapAlloc,
						"heap_sys":          m.HeapSys,
						"heap_idle":         m.HeapIdle,
						"heap_inuse":        m.HeapInuse,
						"heap_released":     m.HeapReleased,
						"heap_objects":      m.HeapObjects,
						"stack_inuse":       m.StackInuse,
						"gc_count":          m.NumGC,
						"gc_total_ns":       m.PauseTotalNs,
						"gc_avg_alltime_ns": gcAlltime,
						"gc_avg_last256_ns": avg256Pause,
					}.Log()
					select {
					case <-stop:
						wg.Done()
						return
					case <-time.After(time.Minute):
					}
				}
			}()
			return nil
		},
	})
}

// WithSignalWatcher configures a composite service object with a
// component to launch a go routine that traps and handles common OS
// signals, passing control to a handler function.
func WithSignalWatcher(signalHandler func(context.Context, os.Signal), signals ...os.Signal) ConfigFn {
	return WithComponent(&SimpleServiceObject{
		OnStart: func(ctx context.Context) error {
			alog.Debug{
				"action":  "signal_watcher_start",
				"status":  alog.StatusStart,
				"signals": signals,
			}.Log()
			go func() {
				sigchan := make(chan os.Signal, 1)
				signal.Notify(sigchan, signals...)
				for {
					sig := <-sigchan
					alog.Info{"action": "trapped_signal", "signal": sig}.Log()
					signalHandler(ctx, sig)
				}
			}()
			return nil
		},
	})
}

// WithShutdownSignalWatcher configures a composite service object
// with a component that launches a go routine to trap typical
// shutdown signals and begin an ordered shutdown.
func WithShutdownSignalWatcher() ConfigFn {
	return WithSignalWatcher(func(ctx context.Context, sig os.Signal) {
		alog.Info{
			"action": "shutdown_signal_watcher",
			"status": "signaled",
			"signal": sig,
			"msg":    "Starting shutdown",
		}.Log()
		if s := GetService(ctx); s != nil {
			s.Exit(0)
		} else {
			Exit(0)
		}
	}, syscall.SIGHUP, syscall.SIGTERM, syscall.SIGKILL, syscall.SIGINT)
}

// WithMemoryProfileDumpSignalWatcher configures a composite service
// object that listens for the SIGUSR2 signal and creates a memory
// dump when it receives it.
func WithMemoryProfileDumpSignalWatcher() ConfigFn {
	return WithSignalWatcher(func(ctx context.Context, sig os.Signal) {
		// dump memory profile
		fn := "/tmp/" + filepath.Base(os.Args[0]) + ".memdump"
		f, err := os.Create(fn)
		if err == nil {
			pprof.WriteHeapProfile(f)
			alog.Info{"action": "memory_profile_create", "filename": fn}.Log()
			f.Close()
		} else {
			alog.Error{"action": "memory_profile_create", "status": "file_open_failed", "error": err}.Log()
		}
	}, syscall.SIGUSR2)
}
