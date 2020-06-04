// Copyright 2016 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

/*
Package buildinfo collects build information supplied during the compile/link
phase of a program and makes it accessible and formatable at runtime as
read-only values.

Supported package variables (all strings):
* product
* buildTimeUnix
* commit
* branch
* buildHost
* buildUser

To compile these values into a Go program use the -X ldflags option:

 go build -ldflags \
	"-X github.com/anki/sai-go-util/buildinfo.product=myappname  \
    -X github.com/anki/sai-go-util/buildinfo.buildTimeUnix=$(date +%s) \
    -X github.com/anki/sai-go-util/buildinfo.buildHost=$(hostname) \
    -X github.com/anki/sai-go-util/buildinfo.buildUser=$USER \
    -X github.com/anki/sai-go-util/buildinfo.branch=$(git symbolic-ref --short HEAD) \
    -X github.com/anki/sai-go-util/buildinfo.commit=$(git rev-parse HEAD)" \
	myprogram.go

The program must, of course, import the buildinfo package to make those
values accessible.
*/
package buildinfo

import (
	"fmt"
	"sort"
	"strconv"
	"strings"
	"time"
)

var (
	product       string
	buildTimeUnix string
	commit        string
	branch        string
	buildHost     string
	buildUser     string

	info BuildInfo
)

func init() {
	doInit()
}

// init calls doInit so that it can be called from unit tests
func doInit() {
	info = BuildInfo{
		Product:       product,
		BuildTimeUnix: buildTimeUnix,
		Commit:        commit,
		Branch:        branch,
		BuildHost:     buildHost,
		BuildUser:     buildUser,
	}

	if t, err := strconv.Atoi(buildTimeUnix); err == nil {
		tm := time.Unix(int64(t), 0)
		info.BuildTime = &tm
	}
}

// Info returns a copy of the build information that was compiled into the
// program.
func Info() BuildInfo {
	return info
}

// BuildInfo holds the build information that was supplied at link time.
type BuildInfo struct {
	Product       string     `json:"product,omitempty"`
	BuildTimeUnix string     `json:"build_time_unix,omitempty"`
	BuildTime     *time.Time `json:"build_time,omitempty"`
	Commit        string     `json:"commit,omitempty"`
	Branch        string     `json:"branch,omitempty"`
	BuildHost     string     `json:"build_host,omitempty"`
	BuildUser     string     `json:"build_user,omitempty"`
}

// String returns the build information as a human readable string.
func (i BuildInfo) String() string {
	entry := []string{
		"Build product: ", fmtString(i.Product), "\n",
		"Build time:   ", fmtString(i.fmtTime()), "\n",
		"Build branch: ", fmtString(i.Branch), "\n",
		"Build commit: ", fmtString(i.Commit), "\n",
		"Build host:   ", fmtString(i.BuildHost), "\n",
		"Build user:   ", fmtString(i.BuildUser), "\n",
	}
	return strings.Join(entry, "")
}

// Map returns the build information as a map of human readable strings
func (i BuildInfo) Map() map[string]string {
	return map[string]string{
		"product":         i.Product,
		"build_time":      i.fmtTime(),
		"build_time_unix": i.BuildTimeUnix,
		"branch":          i.Branch,
		"commit":          i.Commit,
		"build_host":      i.BuildHost,
		"build_user":      i.BuildUser,
	}
}

// KVString returns a single-line string of key="value" pairs containing the
// build information.  Keys are ordered alphabetically. Intended to be useful
// in log entries.
func (i BuildInfo) KVString() string {
	result := make([]string, 0, 10)
	keys := make([]string, 0, 10)
	for k := range i.Map() {
		keys = append(keys, k)
	}
	sort.Strings(keys)
	m := i.Map()
	for _, k := range keys {
		result = append(result, fmt.Sprintf("%s=%q", k, m[k]))
	}
	return strings.Join(result, " ")
}

func (i BuildInfo) fmtTime() string {
	if i.BuildTime == nil {
		return ""
	}
	return i.BuildTime.Format(time.RFC3339)
}

// String returns the build information as a human readable string.
func String() string {
	return info.String()
}

// Map returns the build information as a map of human readable strings
func Map() map[string]string {
	return info.Map()
}

// KVString returns a single-line string of key="value" pairs containing the
// build information.  Keys are ordered alphabetically. Intended to be useful
// in log entries.
func KVString() string {
	return info.KVString()
}

// ForceReset overwrites the compiled-in build info values with the
// supplied values.  Intended only for testing and is not goroutine safe.
func ForceReset(newInfo BuildInfo) {
	info = newInfo
}

func fmtString(s string) string {
	if s == "" {
		return "-"
	}
	return s
}
