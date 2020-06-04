// Copyright 2016 Anki, Inc.

// Package alog provides go-value style logging using the standard Go logging library
package alog

// (c) Anki, Inc 2014

import (
	"context"
	"errors"
	"fmt"
	"io"
	"log"
	"log/syslog"
	"net/http"
	"os"
	"runtime"
	"sort"
	"strconv"
	"strings"
	"sync"
	"time"

	gcontext "github.com/gorilla/context"
)

const (
	LogDebug = iota
	LogInfo
	LogWarn
	LogError
	LogPanic

	StatusOK    = "ok"
	StatusError = "error"
	StatusFail  = "fail"
	StatusStart = "start"
	StatusDone  = "done"
)

type AlogFlag int

const (
	LogTime AlogFlag = 1 << iota
)

var (
	LogLevels = [...]string{"DEBUG", "INFO", "WARN", "ERROR", "PANIC"}
)

var (
	Logger   *log.Logger
	StdFlags = 0
	//StdFlags    = log.Ldate | log.Ltime | log.Lmicroseconds | log.LUTC
	SyslogFlags           = log.Llongfile
	KeyOrder              = []string{"commit", "request_id", "client_id", "cmd", "action", "status", "error"}
	MinLogLevel           = LogInfo
	Stdout      io.Writer = os.Stdout
	// The commit hash of the code we're logging before
	Commit string
	// The name of the cli package Command that's currently executing, if we're running one
	Command string

	// If true then every new line in a log entry will be escaped with a backslash (including RawLogString entries)
	EscapeNewlines bool     = false
	AlogFlags      AlogFlag = LogTime
)

// LogMap defines a synchronized map and is
// safe to be used by multiple go-routines.
// The built-in sync.Map has been considered and not been used
// due to performance concerns.
type LogMap struct {
	m     sync.RWMutex // protects the store.
	store map[string]interface{}
}

// NewLogMap initializes an empty map.
func NewLogMap() *LogMap {
	return &LogMap{
		store: make(map[string]interface{}),
	}
}

// Load returns the value stored in the map for a key, or nil if no
// value is present.
// The ok result indicates whether value was found in the map.
func (lm *LogMap) Load(key string) (value interface{}, ok bool) {
	lm.m.RLock()
	defer lm.m.RUnlock()
	value, ok = lm.store[key]
	return value, ok
}

// Delete deletes the value for a key.
func (lm *LogMap) Delete(key string) {
	lm.m.Lock()
	defer lm.m.Unlock()
	delete(lm.store, key)
}

// Store sets the value for a key.
func (lm *LogMap) Store(key string, value interface{}) {
	lm.m.Lock()
	defer lm.m.Unlock()
	lm.store[key] = value
}

// Range calls f sequentially for each key and value present in the map.
// If f returns false, range stops the iteration.
func (lm *LogMap) Range(f func(key string, value interface{}) bool) {
	lm.m.RLock()
	defer lm.m.RUnlock()

	for k, v := range lm.store {
		if !f(k, v) {
			break
		}
	}
}

// types implementing the LogValue interface have the result
// of the LogValue method emitted directly into a log string.
//
// No newlines are stripped, and strings are not quoted.
type LogValue interface {
	LogValue() string
}

// RawLogString returns a LogValue so that newlines, etc
// are not stripped and the string is not quoted.
type RawLogString string

func (s RawLogString) LogValue() string {
	return string(s)
}

// types implementing the Loggable interface will be asked
// to supply a set of keys and values to add to the log message
// rather than a single value
type Loggable interface {
	ToLogValues() map[string]interface{}
}

func SetLevelByString(lev string) error {
	lev = strings.ToUpper(lev)
	for l, v := range LogLevels {
		if v == lev {
			MinLogLevel = l
			return nil
		}
	}
	return errors.New("Invalid log level")
}

func ToStdout() {
	Logger = log.New(Stdout, "", StdFlags)
}

func ToWriter(w io.Writer) {
	Logger = log.New(w, "", StdFlags)
}

func ToSyslog(p syslog.Priority, tag string) (err error) {
	s, err := syslog.New(p, tag)
	if err != nil {
		return err
	}
	Logger = log.New(s, "", StdFlags)
	return nil
}

func ToSyslogAddr(addr string, p syslog.Priority, tag string) (err error) {
	protoaddr := strings.SplitN(addr, ":", 2)
	if len(protoaddr) != 2 {
		return errors.New("Bad syslog address")
	}
	s, err := syslog.Dial(protoaddr[0], protoaddr[1], p, tag)
	if err != nil {
		return err
	}
	Logger = log.New(s, "", StdFlags)
	return nil
}

type logWriter struct {
	logLevel int
	action   string
}

func (w logWriter) Write(p []byte) (n int, err error) {
	doLog(w.logLevel, logData{
		"action": w.action,
	}, 4, p)
	return len(p), nil
}

// MakeLogger returns a logger that feeds data into alog at the specified log level
func MakeLogger(logLevel int, action string) *log.Logger {
	w := &logWriter{logLevel: logLevel, action: action}
	return log.New(w, "", 0)
}

// CheckPanic should be called from a defer statement to log any panic the program may
// have generated.  It can either re-raise, or swallow the captured panic.
//
// eg.
//  main() {
//      defer alog.CheckPanic(true)
//
//      doStuff()
//  }
func CheckPanic(reraise bool) {
	if err := recover(); err != nil {
		Panic{"status": "panic", "panic": err}.Log()
		if reraise {
			panic(err)
		}
	}
}

type Interface interface {
	Log()
	LogR(r *http.Request)
	LogC(c context.Context)
}

type TimeIt interface {
	TimeIt(f func() error)
}

type kv struct {
	k string
	v interface{}
}

type kvs []kv

func (kvs kvs) Len() int {
	return len(kvs)
}

func (kvs kvs) Less(i, j int) bool {
	return kvs[i].k < kvs[j].k
}

func (kvs kvs) Swap(i, j int) {
	kvs[i], kvs[j] = kvs[j], kvs[i]
}

func escapeNewlines(buf []byte) []byte {
	buflen := len(buf)
	for i := buflen - 1; i >= 0; i-- {
		ch := buf[i]
		switch ch {
		case '\n':
			buf = append(buf[0:i], append([]byte("\\n"), buf[i+1:]...)...)
		case '\r':
			buf = append(buf[0:i], append([]byte("\\r"), buf[i+1:]...)...)
		}
	}
	return buf
}

func fmtkv(n int, buf []byte, k string, v interface{}) []byte {

	if v, ok := v.(Loggable); ok {
		lmap := v.ToLogValues()
		lpairs := make(kvs, 0, len(lmap))

		// collect keys in the loggable object
		for sk, sv := range lmap {
			lpairs = append(lpairs, kv{k: sk, v: sv})
		}

		// sort keys
		sort.Sort(lpairs)

		// TODO: does not take care of duplicates keys in postPairs

		for _, lpair := range lpairs {
			sk, sv := lpair.k, lpair.v

			_, ok := sv.(Loggable)
			if ok {
				panic("nested loggable objects not supported")
			}

			buf = fmtkv(n+1, buf, k+sk, sv)
		}

		return buf
	}

	if n > -1 {
		buf = append(buf, ' ')
	}
	buf = append(buf, []byte(k)...)
	buf = append(buf, '=')
	switch data := v.(type) {
	case fmt.Stringer:
		buf = append(buf, []byte(fmt.Sprintf("%+q", data))...)
	case string:
		buf = append(buf, []byte(fmt.Sprintf("%+q", data))...)
	case *string:
		if data == nil {
			buf = append(buf, []byte("<nil>")...)
		} else {
			buf = append(buf, []byte(fmt.Sprintf("%+q", *data))...)
		}
	case error:
		buf = append(buf, []byte(fmt.Sprintf("%+q", data.Error()))...)
	case []byte:
		buf = append(buf, []byte(fmt.Sprintf("%+q", data))...)
	case LogValue:
		buf = append(buf, []byte(data.LogValue())...)
	default:
		buf = append(buf, []byte(fmt.Sprintf("%v", data))...)
	}
	return buf
}

func fmtCaller(buf *[]byte, calldepth int) {
	callptr, file, line, ok := runtime.Caller(calldepth + 1)
	if ok {
		if f := runtime.FuncForPC(callptr); f != nil {
			*buf = append(*buf, f.Name()...)
			*buf = append(*buf, ':')
			*buf = append(*buf, strconv.Itoa(line)...)
		} else {
			*buf = append(*buf, file...)
			*buf = append(*buf, ':')
			*buf = append(*buf, strconv.Itoa(line)...)
		}
	} else {
		*buf = append(*buf, "unknown_caller"...)
	}
	*buf = append(*buf, ' ')
}

// lifted from log.go
// Cheap integer to fixed-width decimal ASCII.  Give a negative width to avoid zero-padding.
func itoa(buf *[]byte, i int, wid int) {
	// Assemble decimal in reverse order.
	var b [20]byte
	bp := len(b) - 1
	for i >= 10 || wid > 1 {
		wid--
		q := i / 10
		b[bp] = byte('0' + i - q*10)
		bp--
		i = q
	}
	// i < 10
	b[bp] = byte('0' + i)
	*buf = append(*buf, b[bp:]...)
}

// append RFC3339 formatted time
// 2006-01-02T15:04:05Z07:00
func addTime(buf *[]byte) {
	t := time.Now().UTC()

	year, month, day := t.Date()
	hour, min, sec := t.Clock()
	ms := t.Nanosecond() / int(time.Millisecond)
	itoa(buf, year, 4)
	*buf = append(*buf, '-')
	itoa(buf, int(month), 2)
	*buf = append(*buf, '-')
	itoa(buf, day, 2)
	*buf = append(*buf, 'T')
	itoa(buf, hour, 2)
	*buf = append(*buf, ':')
	itoa(buf, min, 2)
	*buf = append(*buf, ':')
	itoa(buf, sec, 2)
	*buf = append(*buf, '.')
	itoa(buf, ms, 3)
	*buf = append(*buf, 'Z')
	*buf = append(*buf, ' ')
}

func addCommit(data logData) {
	if Commit != "" {
		data["commit"] = Commit
	}
}

func addCommand(data logData) {
	if Command != "" {
		data["cmd"] = Command
	}
}

func doLog(level int, data logData, calldepth int, trailer []byte) {
	if Logger == nil || level < MinLogLevel {
		return
	}
	n := 0
	msg := make([]byte, 0, 100)

	if AlogFlags&LogTime != 0 {
		addTime(&msg)
	}
	addCommit(data)
	addCommand(data)
	fmtCaller(&msg, calldepth+1)

	msg = append(msg, `ll=`...)
	msg = append(msg, LogLevels[level]...)
	msg = append(msg, ' ')

	prePairs := make(kvs, 0, len(KeyOrder))
	postPairs := make(kvs, 0, len(data))

	for _, k := range KeyOrder {
		if v, ok := data[k]; ok {
			prePairs = append(prePairs, kv{k, v})
		}
	}
	for k, v := range data {
		if !seen(k) {
			postPairs = append(postPairs, kv{k, v})
		}
	}

	sort.Sort(postPairs)
	for _, kv := range prePairs {
		msg = fmtkv(n, msg, kv.k, kv.v)
		n++
	}
	for _, kv := range postPairs {
		msg = fmtkv(n, msg, kv.k, kv.v)
		n++
	}
	if EscapeNewlines {
		msg = escapeNewlines(msg)
	}
	if len(trailer) > 0 {
		if trailer[0] != '\n' && trailer[0] != ' ' {
			msg = append(msg, ' ')
		}
		msg = append(msg, trailer...)
	}
	Logger.Print(string(msg))
}

type ctxLoggerKeyType int

const ctxLoggerKey ctxLoggerKeyType = iota

// WithLogMap returns a context with a LogMap instance bound as a context value.
func WithLogMap(parent context.Context, kv map[string]interface{}) context.Context {
	nmap := NewLogMap()
	for k, v := range kv {
		nmap.store[k] = v
	}
	// check if a log map already exists
	lmap, ok := parent.Value(ctxLoggerKey).(*LogMap)
	if ok {
		// make a copy of the existing map
		lmap.Range(func(k string, v interface{}) bool {
			_, isPresent := nmap.store[k]
			// mangle the name with a prefix so that it no longer collides,
			// but can still be found in Splunk (via an alert)
			if isPresent {
				k = "__dupe_name__" + k
			}
			nmap.store[k] = v
			return true
		})
	}
	return context.WithValue(parent, ctxLoggerKey, nmap)
}

// GetLogMap extracts LogMap from the context, or nil if no
// LogMap object have been bound.
func GetLogMap(ctx context.Context) *LogMap {
	value, _ := ctx.Value(ctxLoggerKey).(*LogMap)
	return value
}

// doLogC initializes the context logger.
func doLogC(level int, data logData, ctx context.Context, calldepth int, trailer []byte) {
	if lmap := GetLogMap(ctx); lmap != nil {
		lmap.Range(func(k string, v interface{}) bool {
			data[k] = v
			return true
		})
	}
	doLog(level, data, calldepth+1, trailer)
}

func doLogR(level int, data logData, r *http.Request, calldepth int, trailer []byte) {
	if requestId := gcontext.Get(r, "requestId"); requestId != nil {
		data["request_id"] = requestId
	}
	if clientId := gcontext.Get(r, "clientId"); clientId != nil {
		data["client_id"] = clientId
	}
	doLog(level, data, calldepth+1, trailer)
}

func doTime(f func() error, level int, data logData, calldepth int) {
	start := time.Now()
	if err := f(); err != nil {
		data["error"] = err
	}
	rt := time.Since(start)
	data["runtime_ms"] = int64(rt / time.Millisecond)
	data["runtime"] = rt.String()
	doLog(level, data, calldepth+1, nil)
}

func seen(k string) bool {
	for _, a := range KeyOrder {
		if k == a {
			return true
		}
	}
	return false
}

type logData map[string]interface{}

// Debug logs entries using an DEBUG logging level
type Debug map[string]interface{}

func (d Debug) Log()                     { doLog(LogDebug, logData(d), 1, nil) }
func (d Debug) LogR(r *http.Request)     { doLogR(LogDebug, logData(d), r, 1, nil) }
func (d Debug) LogC(ctx context.Context) { doLogC(LogDebug, logData(d), ctx, 1, nil) }
func (d Debug) TimeIt(f func() error)    { doTime(f, LogDebug, logData(d), 1) }

// Info logs entries using an INFO logging level
type Info map[string]interface{}

func (i Info) Log()                     { doLog(LogInfo, logData(i), 1, nil) }
func (i Info) LogR(r *http.Request)     { doLogR(LogInfo, logData(i), r, 1, nil) }
func (i Info) LogC(ctx context.Context) { doLogC(LogInfo, logData(i), ctx, 1, nil) }
func (i Info) TimeIt(f func() error)    { doTime(f, LogInfo, logData(i), 1) }

// Info logs entries using an WARN logging level
type Warn map[string]interface{}

func (w Warn) Log()                     { doLog(LogWarn, logData(w), 1, nil) }
func (w Warn) LogR(r *http.Request)     { doLogR(LogWarn, logData(w), r, 1, nil) }
func (w Warn) LogC(ctx context.Context) { doLogC(LogWarn, logData(w), ctx, 1, nil) }
func (w Warn) TimeIt(f func() error)    { doTime(f, LogWarn, logData(w), 1) }

// Info logs entries using an WARN logging level
type Error map[string]interface{}

func (e Error) Log()                     { doLog(LogError, logData(e), 1, nil) }
func (e Error) LogR(r *http.Request)     { doLogR(LogError, logData(e), r, 1, nil) }
func (e Error) LogC(ctx context.Context) { doLogC(LogError, logData(e), ctx, 1, nil) }
func (e Error) TimeIt(f func() error)    { doTime(f, LogError, logData(e), 1) }

type Panic map[string]interface{}

func (p Panic) Log() {
	stack := make([]byte, 1<<16)
	stack[0] = '\n'
	stack = stack[:1+runtime.Stack(stack[1:], false)]
	doLog(LogPanic, logData(p), 1, stack)
}

func (p Panic) LogR(r *http.Request) {
	stack := make([]byte, 1<<16)
	stack[0] = '\n'
	stack = stack[:1+runtime.Stack(stack[1:], false)]
	doLogR(LogPanic, logData(p), r, 1, stack)
}

func (p Panic) LogC(ctx context.Context) {
	stack := make([]byte, 1<<16)
	stack[0] = '\n'
	stack = stack[:1+runtime.Stack(stack[1:], false)]
	doLogC(LogPanic, logData(p), ctx, 1, stack)
}
