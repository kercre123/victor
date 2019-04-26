// Copyright 2014-2016 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// Package cli provides utilities for command line applications
package cli

import (
	"fmt"
	"io"
	"os"
	"os/signal"
	"path/filepath"
	"runtime"
	"runtime/pprof"
	"sync"
	"syscall"
	"time"

	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-kinlog/logsender"
	"github.com/anki/sai-kinlog/logsender/logrecord"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials/stscreds"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/gwatts/profile"
	"github.com/gwatts/rootcerts"
)

func init() {
	rootcerts.UpdateDefaultTransport()
}

// LogTargetType specifies what type of logger should be setup.
type LogTargetType string

const (
	// LogToStdout specifies that log output should be setup to direct output to stdout.
	LogToStdout LogTargetType = "stdout"

	// LogToKinesis specifies that log output should be sent to a Kinesis stream.
	LogToKinesis LogTargetType = "kinesis"
)

// SigWatcher runs a goroutine montirong for some common OS signals
//
// On receiving SIGUSR2 it will dump a memory profile to
// /tmp/<process name>.memdump
//
// On any other signal it will call the specified StopHandler.
type SigWatcher struct {
	StopHandler func(sig os.Signal)
}

// Start starts a goroutine to wait for an incoming OS signal.
func (s *SigWatcher) Start() {
	go s.sigwatcher()
}

func (s *SigWatcher) sigwatcher() {
	sigchan := make(chan os.Signal, 1)
	signal.Notify(sigchan, syscall.SIGHUP, syscall.SIGTERM, syscall.SIGKILL, syscall.SIGINT, syscall.SIGUSR2)
	for {
		sig := <-sigchan
		switch sig {
		case syscall.SIGUSR2:
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

		default:
			// All other registered signals will trigger a shutdown
			alog.Info{"action": "trapped_signal", "signal": sig}.Log()
			s.StopHandler(sig)
			return
		}
	}
}

// StartSigWatcher is a convenience function to create a new SigWatcher,
// register a StopHandler with it and call Start.
func StartSigWatcher(onstop func(sig os.Signal)) *SigWatcher {
	s := &SigWatcher{StopHandler: onstop}
	s.Start()
	return s
}

type exitCode int

// Exit is a replacemnt for os.Exit - If ensures log cleanup/flushing is
// performed before exit.
func Exit(code int) {
	panic(exitCode(code))
}

var cleanupHandlers []func()

func registerCleanupHandler(f func()) {
	cleanupHandlers = append(cleanupHandlers, f)
}

// CleanupAndExit will trap and log any panic that was raised and hitherto
// untrapped on the main goroutine, cleanly shutdown log handlers and exit
// with any status code  that was set using Exit.
//
// NOTE: this cannot trap panics raised from other goroutines.
func CleanupAndExit() {
	var ec exitCode

	defer func() {
		if ec != 0 {
			os.Exit(int(ec))
		}
	}()
	defer runCleanupHandlers()

	if err := recover(); err != nil {
		// note the exit code if the panic was raised by a call to Exit
		var ok bool
		if ec, ok = err.(exitCode); ok {
			alog.Info{"status": "exiting", "exit_code": ec}.Log()

		} else {
			// else we've trapped some other panic; log it.
			alog.Panic{"status": "panic", "panic": err}.Log()

			// Ensure panic is still displayed to Stderr once the cleanup
			// handlers have run.
			panic(err)
		}
	}
}

func runCleanupHandlers() {
	// ensure cleanupHandlers only run once.
	defer func() { cleanupHandlers = nil }()

	// run registered cleanup code
	for _, f := range cleanupHandlers {
		defer func(f func()) { f() }(f)
	}
}

var (
	// LogMetadata holds the default base log metadata to send to Kinesis.
	LogMetadata = logrecord.Metadata{
		Sourcetype: "sai_go_general",
	}

	// LogKinesisStream specifies the name of the Kinesis stream to send log
	// data to.
	LogKinesisStream string

	// LogKinesisRoleARN specifies an optional AWS Role to assume when sending
	// log events; used to send log data to a cross-accounts stream.
	LogKinesisRoleARN string

	// LogLevel specifies the base log level to record log events for.  Log
	// entries below this level will be dropped.
	LogLevel = "INFO"

	// LogType specifies the type of log handler to use.
	LogType = LogToStdout

	logType          = string(LogToStdout)
	kinesisLogSender *logsender.Sender
)

// Fail prints an error message and causes the program to exit with an exit
// code of 100.
func Fail(format string, a ...interface{}) {
	fmt.Printf(format+"\n", a...)
	Exit(100)
}

// InitLogFlags should be called from init to setup logging related flags.
func InitLogFlags(defaultSource string) {
	LogMetadata.Source = defaultSource
	envconfig.String(&logType, "LOG_TYPE", "log-type", "Either \"kinesis\" or \"stdout\"")
	envconfig.String(&LogLevel, "LOG_LEVEL", "log-level", "Set logging threshold")
	envconfig.String(&LogKinesisStream, "LOG_KINESIS_STREAM", "log-kinesis-stream", "Kinesis stream to send logs to")
	envconfig.String(&LogKinesisRoleARN, "LOG_KINESIS_ROLE_ARN", "log-kinesis-role-arn", "AWS Role ARN to assume to send log events, if required")
	envconfig.String(&LogMetadata.Source, "LOG_SOURCE", "log-source", "Source to allocate to the general log")
	envconfig.String(&LogMetadata.Sourcetype, "LOG_SOURCETYPE", "log-sourcetype", "Sourcetype to allocate to the general log (default of sai_go_general should not be changed)")
	envconfig.String(&LogMetadata.Index, "LOG_INDEX", "log-index", "Index to allocate to the general log")
}

// SetupLogging connects our logger util to a Kinesis LogSender
// You must call CleanupAndExit() on program exit to ensure that any
// outstanding log data is correctly flushed.
func SetupLogging() {
	if commit := buildinfo.Info().Commit; len(commit) > 8 {
		alog.Commit = commit[:8]
	} else {
		alog.Commit = "unknown"
	}

	if err := alog.SetLevelByString(LogLevel); err != nil {
		Fail("Failed to set log level to %q: %s", LogLevel, err)
	}

	switch logType {
	case string(LogToKinesis):
		LogType = LogToKinesis
		registerCleanupHandler(setupKinesisLogging())

	case string(LogToStdout):
		LogType = LogToStdout
		registerCleanupHandler(setupStdoutLogging())

	default:
		Fail("Unknown logging type")
	}
}

func setupStdoutLogging() (cleanup func()) {
	alog.ToStdout()
	return func() {}
}

// replaced by unit tests
var newKinesisSender = func(stream string, config ...logsender.Config) (*logsender.Sender, error) {
	return logsender.New(stream, config...)
}

func setupKinesisLogging() (cleanup func()) {
	var scfg []logsender.Config

	if LogKinesisRoleARN != "" {
		creds := stscreds.NewCredentials(session.New(), LogKinesisRoleARN, func(arp *stscreds.AssumeRoleProvider) {
			arp.RoleSessionName = filepath.Base(os.Args[0])
			arp.Duration = 1 * time.Hour
			arp.ExpiryWindow = 10 * time.Minute
		})
		sess := session.New(aws.NewConfig().WithCredentials(creds))
		scfg = append(scfg, logsender.WithSession(sess))
	}

	sender, err := newKinesisSender(LogKinesisStream, scfg...)
	if err != nil {
		Fail("Failed to setup Kinesis logger: %s", err)
	}

	// log the kinesis send stats every minute
	monitor := &logsender.Monitor{
		LogInterval: time.Minute,
		Logger:      alog.MakeLogger(alog.LogInfo, "kinlog_stats"),
		Sender:      sender,
	}
	monitor.Start()

	if hostname, err := os.Hostname(); err == nil {
		LogMetadata.Host = hostname
	}

	alog.ToWriter(sender.NewEventWriter(LogMetadata, false))
	kinesisLogSender = sender
	fmt.Printf("Sending log data to Kinesis stream=%s\n", LogKinesisStream)
	return func() {
		fmt.Print("Shutting down Kinesis logger...")
		monitor.Stop()
		sender.Stop()
		fmt.Println("Done.")
	}
}

// NewLogWriter adds a new log stream to write log data to.
// Primarily used for web access logs, which are separate from the general log.
func NewLogWriter(source, sourcetype string) io.Writer {
	switch LogType {
	case LogToStdout:
		return os.Stdout
	case LogToKinesis:
		md := LogMetadata
		md.Source += "_" + source
		md.Sourcetype = sourcetype
		if kinesisLogSender == nil {
			panic("LogType==kinesis, but logSender is uninitialized")
		}
		return kinesisLogSender.NewEventWriter(md, false)
	default:
		panic(fmt.Sprintf("Unknown LogType %q", LogType))
	}
}

// NewAccessLogWriter is a wrapper for NewLogWriter that's hard-wired to use
// the SAI standard sai_access_combined sourcetype suited to our
// the log generated by the sai-go-util/http/httplog package
//
// If source is passed as an empty string it will default to "access_log"
// which will, in turn be prefixed with the source name for the default
// LogMetadata (eg. sai_go_accounts_access_log)
func NewAccessLogWriter(source string) io.Writer {
	if source == "" {
		source = "access_log"
	}
	return NewLogWriter(source, "sai_access_combined")
}

// StartRuntimeLogger starts a goroutine some runtime memory stats every minute
func StartRuntimeLogger() {
	var wg sync.WaitGroup
	stop := make(chan struct{})
	wg.Add(1)

	registerCleanupHandler(func() {
		close(stop)
		wg.Wait()
	})

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
				"action":        "runtime_stats",
				"goroutines":    runtime.NumGoroutine(),
				"mem_in_use":    m.Alloc,
				"mem_allocated": m.TotalAlloc,
				"mem_system":    m.Sys,
				"heap_alloc":    m.HeapAlloc,
				"heap_sys":      m.HeapSys,
				"heap_idle":     m.HeapIdle,
				"heap_inuse":    m.HeapInuse,
				"heap_released": m.HeapReleased,
				"heap_objects":  m.HeapObjects,
				"stack_inuse":   m.StackInuse,
				"gc_count":      m.NumGC, "gc_total_ns": m.PauseTotalNs,
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
}

// Profiler defines the Stop method present on Profiles.
type Profiler interface {
	Stop()
}

// StartProfiler starts a profile of the specified type, recording output
// to a filepath.
//
// type must be one of "mem", "cpu" or "block".
//
// The Stop method on the resulting Profiler should be called to stop
// the profiler running.
func StartProfiler(profilePath, profileType string) Profiler {
	if profilePath == "" {
		profilePath = "/tmp"
	}
	config := []profile.Config{
		profile.NoShutdownHook,
		profile.ProfilePath(profilePath),
	}
	switch profileType {
	case "mem":
		config = append(config, profile.MemProfile)
	case "cpu":
		config = append(config, profile.CPUProfile)
	case "block":
		config = append(config, profile.BlockProfile)
	default:
		return nil
	}
	return profile.Start(config...)
}
