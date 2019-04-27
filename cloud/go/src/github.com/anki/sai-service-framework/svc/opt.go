package svc

import (
	"strings"
	"time"

	"github.com/jawher/mow.cli"
	"github.com/stoewer/go-strcase"
)

// Cli Helpers -- wrappers for the mow.cli option functions that
// support specifying an environment variable.

func toEnv(name string) string {
	return strings.ToUpper(strcase.SnakeCase(name))
}

func BoolOpt(cmd *cli.Cmd, name string, value bool, description string) *bool {
	return cmd.Bool(cli.BoolOpt{
		Name:   name,
		EnvVar: toEnv(name),
		Value:  value,
		Desc:   description,
	})
}

func StringOpt(cmd *cli.Cmd, name, value, description string) *string {
	return cmd.String(cli.StringOpt{
		Name:   name,
		EnvVar: toEnv(name),
		Value:  value,
		Desc:   description,
	})
}

func StringsOpt(cmd *cli.Cmd, name string, value []string, description string) *[]string {
	return cmd.Strings(cli.StringsOpt{
		Name:   name,
		EnvVar: toEnv(name),
		Value:  value,
		Desc:   description,
	})
}

func IntOpt(cmd *cli.Cmd, name string, value int, description string) *int {
	return cmd.Int(cli.IntOpt{
		Name:   name,
		EnvVar: toEnv(name),
		Value:  value,
		Desc:   description,
	})
}

func IntsOpt(cmd *cli.Cmd, name string, value []int, description string) *[]int {
	return cmd.Ints(cli.IntsOpt{
		Name:   name,
		EnvVar: toEnv(name),
		Value:  value,
		Desc:   description,
	})
}

// Duration implements a command line option type for specifying
// durations in human-readble form conforming to go's
// time.ParseDuration() format.
type Duration time.Duration

func (d *Duration) Set(v string) error {
	if value, err := time.ParseDuration(v); err != nil {
		return err
	} else {
		*d = Duration(value)
	}
	return nil
}

func (d Duration) String() string {
	return time.Duration(d).String()
}

func (d Duration) Duration() time.Duration {
	return time.Duration(d)
}

// DurationOpt adds an option to cmd which sets a duration value
// (i.e. 500ms, 10s, 1h, etc...) via command line option or
// environment variable.
func DurationOpt(cmd *cli.Cmd, name string, value time.Duration, description string) *Duration {
	d := Duration(value)
	cmd.Var(cli.VarOpt{
		Name:   name,
		EnvVar: toEnv(name),
		Desc:   description,
		Value:  &d,
	})
	return &d
}
