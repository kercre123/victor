// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// Package envconfig provides an easy way to define configuration
// flags that fall back to environment variables and unifies the
// usage instructions for both sources of data.
package envconfig

import (
	"flag"
	"fmt"
	"io"
	"os"
	"sort"
	"strconv"
	"strings"
	"time"
)

var (
	// DefaultConfig holds a Config objects that's preconfigured and ready to use.
	DefaultConfig = New()
)

type ev struct {
	desc string
	val  interface{}
}

// Config holds information about the configured environment variables.
type Config struct {
	envKeys map[string]ev
	SubCmd  string // Used by Usage() if this is configuration for a sub-command
	Flags   *flag.FlagSet
	Output  io.Writer
}

// New returns a Config object.
//
// Normally you'd just use DefaultConfig directly.
func New() *Config {
	c := &Config{
		envKeys: make(map[string]ev),
		Flags:   flag.NewFlagSet(os.Args[0], flag.ExitOnError),
	}
	c.Flags.Usage = c.Usage
	return c
}

// Print usage instructions.
//
// Defaults to os.Stderr unless Config.Output has been set.
func (c *Config) Usage() {
	f := c.Output
	if f == nil {
		f = os.Stderr
	}
	cmd := os.Args[0]
	if c.SubCmd != "" {
		cmd += " " + c.SubCmd
	}
	fmt.Fprintf(f, "Usage of %s:\n", cmd)
	c.PrintDefaults()
}

func (c *Config) PrintDefaults() {
	f := c.Output
	if f == nil {
		f = os.Stderr
	}
	c.Flags.SetOutput(f)
	c.Flags.PrintDefaults()
	fmt.Fprintln(f, "Environment variables:")
	fmt.Fprintln(f, c.EnvDescription())
}

// Returns information about defined environment variables.
func (c *Config) EnvDescription() string {
	if len(c.envKeys) == 0 {
		return ""
	}

	keys := make([]string, 0, len(c.envKeys))
	result := make([]string, len(c.envKeys))
	maxlen := 0
	for k, _ := range c.envKeys {
		keys = append(keys, k)
		if len(k) > maxlen {
			maxlen = len(k)
		}
	}
	sort.Strings(keys)
	// pad key strings for formatting
	strfmt := fmt.Sprintf("%%%ds - %%s (%%v)", maxlen)
	for i, k := range keys {
		entry := c.envKeys[k]
		result[i] = fmt.Sprintf(strfmt, k, entry.desc, entry.val)
	}
	return strings.Join(result, "\n")
}

// Fetch a string value from a flag or an environment variable.
// The passed in target must hold a valid default value and not be nil.
func (c *Config) String(target *string, envName, flagName, description string) {
	if envName == "" {
		c.Flags.StringVar(target, flagName, *target, description)
		return
	}
	if v := os.Getenv(envName); v != "" {
		*target = v
	}
	if flagName != "" {
		c.Flags.StringVar(target, flagName, *target, fmt.Sprintf("%s (overrides %s env var)", description, envName))
	} else if _, haskey := c.envKeys[envName]; haskey {
		panic("Duplicate environment variable name passed to String: " + envName)
	}
	c.envKeys[envName] = ev{desc: description, val: *target}
}

var (
	StringArrayDelimiter = ","
)

type StringSlice []string

func (s *StringSlice) String() string {
	return fmt.Sprintf("%v", *s)
}

func (s *StringSlice) Set(value string) error {
	*s = strings.Split(value, StringArrayDelimiter)
	return nil
}

// Fetch an array of strings from a flag or an environment variable.
// The passed in target must hold a valid default value and not be nil.
func (c *Config) StringArray(target *StringSlice, envName, flagName, description string) {
	if envName == "" {
		c.Flags.Var(target, flagName, description)
		return
	}
	if v := os.Getenv(envName); v != "" {
		*target = strings.Split(v, StringArrayDelimiter)
	}
	if flagName != "" {
		c.Flags.Var(target, flagName, fmt.Sprintf("%s (overrides %s env var)", description, envName))
	} else if _, haskey := c.envKeys[envName]; haskey {
		panic("Duplicate environment variable name passed to StringArray: " + envName)
	}
	c.envKeys[envName] = ev{desc: description, val: *target}
}

// Fetch an int value from a flag or an environment variable.
// The passed in target must hold a valid default value and not be nil.
func (c *Config) Int(target *int, envName, flagName string, description string) {
	if envName == "" {
		c.Flags.IntVar(target, flagName, *target, description)
		return
	}
	if v, err := strconv.Atoi(os.Getenv(envName)); err == nil {
		*target = v
	}
	if flagName != "" {
		c.Flags.IntVar(target, flagName, *target, fmt.Sprintf("%s (overrides %s env var)", description, envName))
	} else if _, haskey := c.envKeys[envName]; haskey {
		panic("Duplicate environment variable name passed to Int: " + envName)
	}
	c.envKeys[envName] = ev{desc: description, val: *target}
}

// Fetch an int64 value from a flag or an environment variable.
// The passed in target must hold a valid default value and not be nil.
func (c *Config) Int64(target *int64, envName, flagName string, description string) {
	if envName == "" {
		c.Flags.Int64Var(target, flagName, *target, description)
		return
	}
	if v, err := strconv.ParseInt(os.Getenv(envName), 10, 64); err == nil {
		*target = v
	}
	if flagName != "" {
		c.Flags.Int64Var(target, flagName, *target, fmt.Sprintf("%s (overrides %s env var)", description, envName))
	} else if _, haskey := c.envKeys[envName]; haskey {
		panic("Duplicate environment variable name passed to Int64: " + envName)
	}
	c.envKeys[envName] = ev{desc: description, val: *target}
}

// Fetch a boolean value from a flag or an environment variable.
// The passed in target must hold a valid default value and not be nil.
//
// An environment variable set to 1, t, T, TRUE, true, True will be set the value to true
// An environment variable set to 0, f, F, FALSE, false, False will set the value to false
// Any other value will leave the default unchanged.
func (c *Config) Bool(target *bool, envName, flagName string, description string) {
	if envName == "" {
		c.Flags.BoolVar(target, flagName, *target, description)
		return
	}
	if b, err := strconv.ParseBool(os.Getenv(envName)); err == nil {
		*target = b
	}
	if flagName != "" {
		c.Flags.BoolVar(target, flagName, *target, fmt.Sprintf("%s (overrides %s env var)", description, envName))
	} else if _, haskey := c.envKeys[envName]; haskey {
		panic("Duplicate environment variable name passed to Bool: " + envName)
	}
	c.envKeys[envName] = ev{desc: description, val: *target}
}

// Fetch a durationvalue from a flag or an environment variable.
// The passed in target must hold a valid default value and not be nil.
func (c *Config) Duration(target *time.Duration, envName, flagName string, description string) {
	if envName == "" {
		c.Flags.DurationVar(target, flagName, *target, description)
		return
	}
	if v, err := time.ParseDuration(os.Getenv(envName)); err == nil {
		*target = v
	}
	if flagName != "" {
		c.Flags.DurationVar(target, flagName, *target, fmt.Sprintf("%s (overrides %s env var)", description, envName))
	} else if _, haskey := c.envKeys[envName]; haskey {
		panic("Duplicate environment variable name passed to Int: " + envName)
	}
	c.envKeys[envName] = ev{desc: description, val: *target}
}

// Wrapper for Config.EnvDescription for the DefaultConfig
func EnvDescription() string {
	return DefaultConfig.EnvDescription()
}

// Wrapper for Config.String for the DefaultConfig
func String(target *string, envName, flagName, description string) {
	DefaultConfig.String(target, envName, flagName, description)
}

// Wrapper for Config.StringArray for the DefaultConfig
func StringArray(target *StringSlice, envName, flagName, description string) {
	DefaultConfig.StringArray(target, envName, flagName, description)
}

// Wrapper for Config.Int for the DefaultConfig
func Int(target *int, envName, flagName, description string) {
	DefaultConfig.Int(target, envName, flagName, description)
}

// Wrapper for Config.Int64 for the DefaultConfig
func Int64(target *int64, envName, flagName, description string) {
	DefaultConfig.Int64(target, envName, flagName, description)
}

// Wrapper for Config.Bool for the DefaultConfig
func Bool(target *bool, envName, flagName, description string) {
	DefaultConfig.Bool(target, envName, flagName, description)
}

// Wrapper for Config.Duration for the DefaultConfig
func Duration(target *time.Duration, envName, flagName, description string) {
	DefaultConfig.Duration(target, envName, flagName, description)
}

// Wrapper for Config.Parse for the DefaultConfig
func Parse() {
	DefaultConfig.Flags.Parse(os.Args[1:])
}

func Usage() {
	DefaultConfig.Usage()
}

func PrintDefaults() {
	DefaultConfig.PrintDefaults()
}
