// Copyright 2016 Anki, Inc.

package alog_test

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"io/ioutil"
	"log"
	"math"
	"net/http"
	"runtime"
	"strconv"
	"strings"
	"testing"
	"time"

	. "github.com/anki/sai-go-util/log"
	. "github.com/anki/sai-go-util/log/logtest"
)

var (
	str       = "string"
	strptr    = &str
	nilstrptr *string
)

type stringerTest struct{}

func (s stringerTest) String() string {
	return "stringer!"
}

var logTests = []struct {
	input    Interface
	expected string
}{
	{Debug{"a": "string"}, "ll=DEBUG  a=\"string\"\n"},
	{Info{"a": "string"}, "ll=INFO  a=\"string\"\n"},
	{Warn{"a": "string"}, "ll=WARN  a=\"string\"\n"},
	{Error{"a": "string"}, "ll=ERROR  a=\"string\"\n"},
	{Panic{"a": "string"}, "ll=PANIC  a=\"string\"\n"},
	{Info{"a": strptr}, "ll=INFO  a=\"string\"\n"},
	{Info{"a": nilstrptr}, "ll=INFO  a=<nil>\n"},
	{Info{"a": []byte("string")}, "ll=INFO  a=\"string\"\n"},
	{Info{"a": 42}, "ll=INFO  a=42\n"},
	{Info{"a": 42.1}, "ll=INFO  a=42.1\n"},
	{Info{"a": math.Inf(1)}, "ll=INFO  a=+Inf\n"},
	{Info{"a": []int{1, 2, 3}}, "ll=INFO  a=[1 2 3]\n"},
	{Info{"a": stringerTest{}}, "ll=INFO  a=\"stringer!\"\n"},
	{Info{"a": errors.New("error test!")}, "ll=INFO  a=\"error test!\"\n"},
}

func TestLog(t *testing.T) {
	oldLevel := MinLogLevel
	defer func() { MinLogLevel = oldLevel }()
	MinLogLevel = LogDebug
	for _, tst := range logTests {
		result := Capture(0, func() {
			tst.input.Log()
		})
		// strip func info prefix
		lines := strings.SplitAfter(result, "\n")
		result = StripFuncName(lines[0])
		if result != tst.expected {
			t.Errorf("expected=%q actual=%q", tst.expected, result)
		}
	}
}

func isDigits(s string) bool {
	for _, ch := range []byte(s) {
		if ch < '0' || ch > '9' {
			return false
		}
	}
	return true
}

func TestTimeIt(t *testing.T) {
	defer func(oldLevel int) { MinLogLevel = oldLevel }(MinLogLevel)
	MinLogLevel = LogDebug
	d := Debug{"a": "string"}
	i := Info{"a": "string"}
	w := Warn{"a": "string"}
	e := Error{"a": "string"}
	for _, l := range []TimeIt{d, i, w, e} {
		result := Capture(0, func() {
			l.TimeIt(func() error { time.Sleep(time.Millisecond); return nil })
		})
		result = StripFuncName(result)
		kv := ExtractKeys(result)
		if missing := HasKeys(kv, "a", "runtime", "runtime_ms"); len(missing) > 0 {
			t.Errorf("Incorrect keys for %#v missing=%#v : %s", l, missing, result)
		}
		if _, ok := kv["error"]; ok {
			t.Error("Unexpected error key set")
		}

		result = Capture(0, func() {
			l.TimeIt(func() error { time.Sleep(time.Millisecond); return errors.New("test error") })
		})
		result = StripFuncName(result)
		kv = ExtractKeys(result)
		if missing := HasKeys(kv, "a", "runtime", "runtime_ms", "error"); len(missing) > 0 {
			t.Errorf("Incorrect keys for %#v missing=%#v : %s", l, missing, result)
		}

		if ms := kv["runtime_ms"]; !isDigits(ms) {
			t.Errorf("Expected runtime_ms to contain only digits, got %q", ms)
		}

	}
}

func TestOrder(t *testing.T) {
	entry := Capture(0, func() { Info{"zero": 0, "one": 1, "status": 2, "three": 3, "action": 4}.Log() })
	expected := "ll=INFO  action=4 status=2 one=1 three=3 zero=0\n"
	entry = StripFuncName(entry)
	if entry != expected {
		t.Errorf("key-value pairs out of order expected=%q actual=%q", expected, entry)
	}
}

func TestNilLogger(t *testing.T) {
	orgLogger := Logger
	Logger = nil
	defer func() { Logger = orgLogger }()
	Info{"a": "b"}.Log()
}

func TestMinLogLevel(t *testing.T) {
	oldLevel := MinLogLevel
	defer func() { MinLogLevel = oldLevel }()
	MinLogLevel = LogInfo
	entry := Capture(0, func() { Debug{"level": "debug"}.Log() })
	if entry != "" {
		t.Errorf("Debug level emitted entry when it shouldn't: %q", entry)
	}
	entry = Capture(0, func() { Info{"level": "info"}.Log() })
	entry = StripFuncName(entry)
	if entry != `ll=INFO  level="info"`+"\n" {
		t.Errorf("Info level emitted incorrect text %q", entry)
	}
}

// Check that the call to Log or LogR records the correct calling
// function and line number.
func TestCallDepth(t *testing.T) {
	defer func(oldLogger *log.Logger, oldFlags AlogFlag) {
		Logger = oldLogger
		AlogFlags = oldFlags
	}(Logger, AlogFlags)

	Info{"status": "test"}.Log()

	r, _ := http.NewRequest("", "", nil)

	data := map[string]interface{}{"foo": "bar"}
	ctx := WithLogMap(context.Background(), data)

	ltypes := []string{"", "context", "request"}

	for _, ltype := range ltypes {
		w := &bytes.Buffer{}
		baseLogger := log.New(w, "", 0)
		Logger = baseLogger
		AlogFlags = 0

		// identify this func
		caller, _, line, _ := runtime.Caller(0)
		f := runtime.FuncForPC(caller)

		switch ltype {
		case "context":
			Info{"status": "test"}.LogC(ctx)
		case "request":
			Info{"status": "test"}.LogR(r)
		default:
			Info{"status": "test"}.Log()
		}

		result := w.String()
		parts := strings.SplitN(result, " ", 2)
		parts = strings.SplitN(parts[0], ":", 2)
		logLine, _ := strconv.Atoi(parts[1])

		if !strings.HasPrefix(result, f.Name()) {
			t.Errorf("expected prefix of %q actual=%q", f.Name(), result)
		}

		// 10 accounts for the number of lines between the time this function called
		// runtime.CAller() and the time we actually called Log() or LogR()
		if math.Abs(float64(logLine-line)) > 10 {
			t.Errorf("line number is incorrect.  Expected close to %d, actual %d", line, logLine)
		}
	}

}

var logNlEscapeTests = []struct {
	enabled  bool
	input    Interface
	expected string
}{
	{true, Info{"entry": RawLogString("one\ntwo\nthree")}, `ll=INFO  entry=one\ntwo\nthree` + "\n"},
	{false, Info{"entry": RawLogString("one\ntwo\nthree")}, "ll=INFO  entry=one\ntwo\nthree\n"},
}

func TestLogEscapeNewlines(t *testing.T) {
	defer func(oldValue bool) { EscapeNewlines = oldValue }(EscapeNewlines)
	for _, test := range logNlEscapeTests {
		EscapeNewlines = test.enabled
		entry := Capture(0, func() { test.input.Log() })
		entry = StripFuncName(entry)

		if entry != test.expected {
			t.Errorf("enabled=%t Mismatch expected=%q actual=%q", test.enabled, test.expected, entry)
		}
	}
}

func TestCommitInfo(t *testing.T) {
	defer func() { Commit = "" }()
	Commit = "somehash"
	entry := Capture(0, func() { Info{"action": "test"}.Log() })
	entry = StripFuncName(entry)
	expected := `ll=INFO  commit="somehash" action="test"` + "\n"
	if entry != expected {
		t.Errorf("expected=%q actual=%q", expected, entry)
	}
}

func TestCommand(t *testing.T) {
	defer func() { Command = "" }()
	Command = "do-something"
	entry := Capture(0, func() { Info{"action": "test"}.Log() })
	entry = StripFuncName(entry)
	expected := `ll=INFO  cmd="do-something" action="test"` + "\n"
	if entry != expected {
		t.Errorf("expected=%q actual=%q", expected, entry)
	}
}

func TestDateFormat(t *testing.T) {
	now := time.Now()
	entry := Capture(LogTime, func() { Info{"action": "test"}.Log() })
	layout := "2006-01-02T15:04:05.000Z"
	ts := entry[:len(layout)]
	et, err := time.Parse(layout, ts)
	if err != nil {
		t.Fatalf("Failed to parse time %q from entry %q: %s", ts, entry, err)
	}
	maxDelta := 10 * time.Millisecond
	if delta := et.Sub(now); delta > maxDelta || delta < -maxDelta {
		t.Errorf("Time mismatch expected=%s actual=%s for timestamp=%q", now, et, ts)
	}
}

func TestMakeLogger(t *testing.T) {
	defer func(oldValue bool) { EscapeNewlines = oldValue }(EscapeNewlines)
	EscapeNewlines = false
	l := MakeLogger(LogInfo, "loggertest")
	result := Capture(0, func() {
		l.Println(`a "quoted" test entry`)
	})
	result = StripFuncName(result)
	expected := `ll=INFO  action="loggertest" a "quoted" test entry` + "\n"
	if result != expected {
		t.Errorf("expected=%q actual=%q", expected, result)
	}
}

func validatePanicLog(s string) error {
	lines := strings.Split(s, "\n")
	if len(lines) < 6 {
		return errors.New("Too few lines to include stack details")
	}
	if !strings.HasPrefix(lines[1], "goroutine ") {
		return errors.New("Expected line 2 to start with goroutine number for stack dump")
	}
	return nil
}

func TestPanic(t *testing.T) {
	defer func(l int) { MinLogLevel = l }(MinLogLevel)
	MinLogLevel = LogDebug

	data := map[string]interface{}{"foo": "bar"}
	ctx := WithLogMap(context.Background(), data)

	r, _ := http.NewRequest("", "", nil)
	calls := []func(){
		func() { Panic{"key": "value"}.Log() },
		func() { Panic{"key": "value"}.LogR(r) },
		func() { Panic{"key": "value"}.LogC(ctx) },
	}

	for i, call := range calls {
		result := Capture(0, call)
		if err := validatePanicLog(result); err != nil {
			t.Error(i, err)
		}
	}
}

func TestCheckPanic(t *testing.T) {
	for _, shouldPanic := range []bool{true, false} {
		doit := func() {
			fmt.Println("shouldPanic", shouldPanic)
			defer CheckPanic(shouldPanic)
			panic("test panic")
		}
		var didPanic interface{}
		buf := new(bytes.Buffer)
		func() {
			defer func() { didPanic = recover() }()
			Runlog(0, buf, doit)
			//result = Capture(0, doit)
		}()

		result := buf.String()

		if shouldPanic && didPanic == nil {
			t.Error("CheckPanic did not re-raise panic")
		} else if !shouldPanic && didPanic != nil {
			t.Errorf("CheckPanic raised an unexpected panic: %v", didPanic)
		}

		if err := validatePanicLog(result); err != nil {
			fmt.Println(result)
			t.Errorf("shouldpanic=%t validation failed for text=%q error=%s", shouldPanic, result, err)
		}
	}
}

type logable map[string]interface{}

func (l logable) ToLogValues() map[string]interface{} {
	return l
}

func TestLogable(t *testing.T) {
	data := logable{
		"key1": "val1",
		"key2": errors.New("val2"),
	}

	entry := Capture(0, func() {
		Info{"other_key": "other_value", "lg_": data}.Log()
	})
	entry = StripFuncName(entry)
	expected := `ll=INFO  lg_key1="val1" lg_key2="val2" other_key="other_value"` + "\n"
	if entry != expected {
		t.Errorf("expected=%q actual=%q", expected, entry)
	}
}

func TestContextLogger(t *testing.T) {
	data := map[string]interface{}{
		"key1": "val1",
		"key2": errors.New("val2"),
	}

	ctx := WithLogMap(context.Background(), data)

	entry := Capture(0, func() {
		Info{"other_key": "other_value"}.LogC(ctx)
	})

	entry = StripFuncName(entry)
	expected := `ll=INFO  key1="val1" key2="val2" other_key="other_value"` + "\n"
	if entry != expected {
		t.Errorf("expected=%q actual=%q", expected, entry)
	}
}

func TestContextLoggerWithMerge(t *testing.T) {
	data := map[string]interface{}{
		"key1": "val1",
		"key2": errors.New("val2"),
	}

	ctx := WithLogMap(context.Background(), data)

	data2 := map[string]interface{}{
		"key3": "val3",
		"key4": errors.New("val4"),
	}

	ctx = WithLogMap(ctx, data2)
	entry := Capture(0, func() {
		Info{"other_key": "other_value"}.LogC(ctx)
	})

	entry = StripFuncName(entry)
	expected := `ll=INFO  key1="val1" key2="val2" key3="val3" key4="val4" other_key="other_value"` + "\n"
	if entry != expected {
		t.Errorf("expected=%q actual=%q", expected, entry)
	}
}

func TestContextLoggerWithDupKey(t *testing.T) {
	data := map[string]interface{}{
		"key1": "val1",
		"key2": errors.New("val2"),
	}
	ctx := WithLogMap(context.Background(), data)

	data2 := map[string]interface{}{
		"key3": "val3",
		"key4": errors.New("val4"),
	}
	ctx = WithLogMap(ctx, data2)

	data3 := map[string]interface{}{
		"key3": "XXX-XXX",
	}
	ctx = WithLogMap(ctx, data3)

	entry := Capture(0, func() {
		Info{"other_key": "other_value"}.LogC(ctx)
	})

	entry = StripFuncName(entry)
	expected := `ll=INFO  __dupe_name__key3="val3" key1="val1" key2="val2" key3="XXX-XXX" key4="val4" other_key="other_value"` + "\n"
	if entry != expected {
		t.Errorf("expected=%q actual=%q", expected, entry)
	}
}

// TODO: Add request context tests

func BenchmarkLog(b *testing.B) {
	orgLogger := Logger
	baseLogger := log.New(ioutil.Discard, "", 0)
	Logger = baseLogger
	defer func() { Logger = orgLogger }()

	for i := 0; i < b.N; i++ {
		Info{"one": "one", "two": "two", "three": 3}.Log()
	}
}
