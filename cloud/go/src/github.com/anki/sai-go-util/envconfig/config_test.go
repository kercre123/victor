package envconfig

import (
	"bytes"
	"flag"
	"os"
	"reflect"
	"strings"
	"testing"
	"time"
)

var stringTests = []struct {
	envname  string
	envval   string
	cmdline  []string
	expected string
}{
	{"", "", []string{""}, "default"},
	{"", "", []string{"-str", "flagval"}, "flagval"},
	{"STR", "envval", []string{"-str", "flagval"}, "flagval"},
	{"STR", "envval", []string{""}, "envval"},
}

func TestStrings(t *testing.T) {
	var org = DefaultConfig
	defer func() { DefaultConfig = org }()

	for _, test := range stringTests {
		val := "default"
		c := New()
		DefaultConfig = c
		c.Flags = flag.NewFlagSet("test", flag.ContinueOnError)
		String(&val, "STR", "str", "description")
		c.Flags.Parse(test.cmdline)
		os.Setenv(test.envname, test.envval)
		if val != test.expected {
			t.Errorf("test=%v expected=%q actual=%q", test, test.expected, val)
		}
	}
}

var stringArrayTests = []struct {
	envname  string
	envval   string
	cmdline  []string
	expected StringSlice
}{
	{"", "", []string{""}, StringSlice{"default"}},
	{"", "", []string{"-arr", "flagval"}, StringSlice{"flagval"}},
	{"", "", []string{"-arr", "f1,f2,f3"}, StringSlice{"f1", "f2", "f3"}},
	{"ARR", "envval", []string{"-arr", "argval"}, StringSlice{"argval"}},
	{"ARR", "e1,e2,e3", []string{""}, StringSlice{"e1", "e2", "e3"}},
}

func TestStringArrays(t *testing.T) {
	defer func(org *Config) { DefaultConfig = org }(DefaultConfig)

	for _, test := range stringArrayTests {
		val := StringSlice{"default"}
		c := New()
		DefaultConfig = c
		c.Flags = flag.NewFlagSet("test", flag.ContinueOnError)
		os.Setenv(test.envname, test.envval)
		StringArray(&val, "ARR", "arr", "description")
		c.Flags.Parse(test.cmdline)
		if !reflect.DeepEqual(val, test.expected) {
			t.Errorf("test=%v expected=%q actual=%q", test, test.expected, val)
		}
	}
}

var intTests = []struct {
	envname  string
	envval   string
	cmdline  []string
	expected int
}{
	{"", "", []string{""}, 42},
	{"", "", []string{"-int", "100"}, 100},
	{"INT", "200", []string{"-int", "100"}, 100},
	{"INT", "200", []string{""}, 200},
}

func TestInts(t *testing.T) {
	var org = DefaultConfig
	defer func() { DefaultConfig = org }()

	for _, test := range intTests {
		val := 42
		c := New()
		DefaultConfig = c
		c.Flags = flag.NewFlagSet("test", flag.ContinueOnError)
		Int(&val, "INT", "int", "description")
		c.Flags.Parse(test.cmdline)
		os.Setenv(test.envname, test.envval)
		if val != test.expected {
			t.Errorf("test=%v expected=%d actual=%d", test, test.expected, val)
		}
	}
}

func TestDuplicateEnvInt(t *testing.T) {
	var err interface{}
	target := 0
	c := New()
	func() {
		defer func() { err = recover() }()
		c.Int(&target, "ENVNAME", "", "")
		c.Int(&target, "ENVNAME", "", "")
	}()
	if err == nil {
		t.Errorf("Int() didn't panic on duplicate")
	}
}

var int64Tests = []struct {
	envname  string
	envval   string
	cmdline  []string
	expected int64
}{
	{"", "", []string{""}, 9223372036854775807},
	{"", "", []string{"-int64", "9223372036854775807"}, 9223372036854775807},
	{"INT64", "9223372036854775807", []string{"-int64", "9223372036854775807"}, 9223372036854775807},
	{"INT64", "9223372036854775807", []string{""}, 9223372036854775807},
}

func TestInt64s(t *testing.T) {
	var org = DefaultConfig
	defer func() { DefaultConfig = org }()

	for _, test := range int64Tests {
		var val int64 = 9223372036854775807
		c := New()
		DefaultConfig = c
		c.Flags = flag.NewFlagSet("test", flag.ContinueOnError)
		Int64(&val, "INT64", "int64", "description")
		c.Flags.Parse(test.cmdline)
		os.Setenv(test.envname, test.envval)
		if val != test.expected {
			t.Errorf("test=%v expected=%d actual=%d", test, test.expected, val)
		}
	}
}

func TestDuplicateEnvInt64(t *testing.T) {
	var err interface{}
	target := int64(0)
	c := New()
	func() {
		defer func() { err = recover() }()
		c.Int64(&target, "ENVNAME", "", "")
		c.Int64(&target, "ENVNAME", "", "")
	}()
	if err == nil {
		t.Errorf("Int64() didn't panic on duplicate")
	}
}

var boolTests = []struct {
	envname    string
	envval     string
	cmdline    []string
	defaultVal bool
	expected   bool
}{
	{"", "", []string{""}, false, false},
	{"", "", []string{"-bool"}, false, true},
	{"BOOL", "T", []string{"-bool=false"}, false, false},
	{"BOOL", "T", []string{""}, false, true},
	{"BOOL", "F", []string{""}, true, false},
	{"BOOL", "ILLEGAL1", []string{""}, true, true},
	{"BOOL", "ILLEGAL2", []string{""}, false, false},
}

func TestBools(t *testing.T) {
	var org = DefaultConfig
	defer func() { DefaultConfig = org }()

	for _, test := range boolTests {
		val := test.defaultVal
		c := New()
		DefaultConfig = c
		c.Flags = flag.NewFlagSet("test", flag.ContinueOnError)
		os.Setenv(test.envname, test.envval)
		Bool(&val, "BOOL", "bool", "description")
		c.Flags.Parse(test.cmdline)
		if val != test.expected {
			t.Errorf("test=%v expected=%t actual=%t", test, test.expected, val)
		}
	}
}

var durationTests = []struct {
	envname  string
	envval   string
	cmdline  []string
	expected time.Duration
}{
	{"", "", []string{""}, time.Hour},
	{"", "", []string{"-dur", "2m5s"}, 125 * time.Second},
	{"DUR", "35s", []string{"-dur", "2m5s"}, 125 * time.Second},
	{"DUR", "35s", []string{}, 35 * time.Second},
}

func TestDurations(t *testing.T) {
	var org = DefaultConfig
	defer func() { DefaultConfig = org }()

	for _, test := range durationTests {
		val := time.Hour
		c := New()
		DefaultConfig = c
		c.Flags = flag.NewFlagSet("test", flag.ContinueOnError)
		os.Setenv(test.envname, test.envval)
		Duration(&val, "DUR", "dur", "description")
		c.Flags.Parse(test.cmdline)
		if val != test.expected {
			t.Errorf("test=%v expected=%s actual=%s", test, test.expected, val)
		}
	}
}

func TestDuplicateEnvString(t *testing.T) {
	var err interface{}
	var target string
	c := New()
	func() {
		defer func() { err = recover() }()
		c.String(&target, "ENVNAME", "", "")
		c.String(&target, "ENVNAME", "", "")
	}()
	if err == nil {
		t.Errorf("String() didn't panic on duplicate")
	}
}

var noEnvTests = []struct {
	name   string
	dotest func(c *Config, setFlag, setEnv bool) bool
}{
	{"String", func(c *Config, setFlag, setEnv bool) bool {
		target, flagname, envname, cmdline := "", "", "", []string{}
		if setFlag {
			flagname, cmdline = "flag", []string{"-flag", "set"}
		}
		if setEnv {
			os.Setenv("ENVTEST", "set")
			envname = "ENVTEST"
		}
		c.String(&target, envname, flagname, "desc")
		c.Flags.Parse(cmdline)
		return target == "set"
	}},
	{"Bool", func(c *Config, setFlag, setEnv bool) bool {
		target, flagname, envname, cmdline := false, "", "", []string{}
		if setFlag {
			flagname, cmdline = "flag", []string{"-flag"}
		}
		if setEnv {
			os.Setenv("ENVTEST", "true")
			envname = "ENVTEST"
		}
		c.Bool(&target, envname, flagname, "desc")
		c.Flags.Parse(cmdline)
		return target
	}},
	{"Int", func(c *Config, setFlag, setEnv bool) bool {
		target, flagname, envname, cmdline := 0, "", "", []string{}
		if setFlag {
			flagname, cmdline = "flag", []string{"-flag", "1"}
		}
		if setEnv {
			os.Setenv("ENVTEST", "1")
			envname = "ENVTEST"
		}
		c.Int(&target, envname, flagname, "desc")
		c.Flags.Parse(cmdline)
		return target == 1
	}},
	{"Int64", func(c *Config, setFlag, setEnv bool) bool {
		target, flagname, envname, cmdline := int64(7223372036854775807), "", "", []string{}
		if setFlag {
			flagname, cmdline = "flag", []string{"-flag", "8223372036854775807"}
		}
		if setEnv {
			os.Setenv("ENVTEST", "8223372036854775807")
			envname = "ENVTEST"
		}
		c.Int64(&target, envname, flagname, "desc")
		c.Flags.Parse(cmdline)
		return target == int64(8223372036854775807)
	}},
}

func TestNoEnvOrFlag(t *testing.T) {
	// defining an entry without an environment variable or a flag name is ok
	var org = DefaultConfig
	defer func() { DefaultConfig = org }()

	for _, test := range noEnvTests {
		for _, envSet := range []bool{true, false} {
			for _, flagSet := range []bool{true, false} {
				c := New()
				DefaultConfig = c
				c.Flags = flag.NewFlagSet("test", flag.PanicOnError)
				if !envSet && !flagSet {
					if test.dotest(c, flagSet, envSet) {
						t.Errorf("Value not set for  envSet=%t flagSet=%t", envSet, flagSet)
					}
				} else {
					if !test.dotest(c, flagSet, envSet) {
						t.Errorf("Value not set for  envSet=%t flagSet=%t", envSet, flagSet)
					}
				}
			}
		}
	}

}

// These indented param descriptions have four spaces and one tab
// (which looks identical to two tabs); if we're going to have a unit
// test for formatting we should make the whitespace sane.
var expectedUsage = `
-boolflag1
    	booldesc (overrides BOOLENV1 env var) (default true)
  -int64flag1 int
    	int64desc (overrides INT64ENV1 env var) (default 9223372036854775807)
  -intflag1 int
    	intdesc (overrides INTENV1 env var) (default 42)
  -strflag1 string
    	strdesc (overrides STRENV1 env var) (default "default_string")
Environment variables:
 BOOLENV1 - booldesc (true)
 BOOLENV2 - booldesc (true)
INT64ENV1 - int64desc (9223372036854775807)
INT64ENV2 - int64desc (9223372036854775807)
  INTENV1 - intdesc (42)
  INTENV2 - intdesc (42)
  STRENV1 - strdesc (default_string)
  STRENV2 - strdesc (default_string)
`

func TestUsage(t *testing.T) {
	var org = DefaultConfig
	defer func() { DefaultConfig = org }()

	buf := &bytes.Buffer{}
	s := "default_string"
	d := 42
	var u int64 = 9223372036854775807
	b := true
	c := New()
	DefaultConfig = c
	c.Output = buf
	c.Flags = flag.NewFlagSet("test", flag.ContinueOnError)
	c.String(&s, "STRENV1", "strflag1", "strdesc")
	c.String(&s, "STRENV2", "", "strdesc")
	c.Int(&d, "INTENV1", "intflag1", "intdesc")
	c.Int(&d, "INTENV2", "", "intdesc")
	c.Int64(&u, "INT64ENV1", "int64flag1", "int64desc")
	c.Int64(&u, "INT64ENV2", "", "int64desc")
	c.Bool(&b, "BOOLENV1", "boolflag1", "booldesc")
	c.Bool(&b, "BOOLENV2", "", "booldesc")
	Usage()
	result := strings.SplitN(buf.String(), "\n", 2)
	if !strings.HasPrefix(result[0], "Usage of") {
		t.Errorf("output should begin \"Usage Of\" - actual=%q", result[0])
	}
	var expected = strings.TrimSpace(expectedUsage)
	var actual = strings.TrimSpace(result[1])
	if actual != expected {
		t.Errorf("output doesn't match.\nExpected:\n%s\nActual:\n%s\n",
			expected, actual)
	}
}
