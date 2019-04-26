// Copyright 2016 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package buildinfo

import (
	"encoding/json"
	"fmt"
	"reflect"
	"strings"
	"testing"
	"time"
)

var (
	testTimeUnix    int64 = 1459473825
	testTimeUnixStr       = "1459473825"
	fmtTestTime           = time.Unix(testTimeUnix, 0).Format(time.RFC3339)
)

func reset() {
	product = ""
	buildTimeUnix = ""
	commit = ""
	branch = ""
	buildHost = ""
	buildUser = ""
	info = BuildInfo{}
	doInit()
}

func setValues() {
	product = "my-product"
	buildTimeUnix = testTimeUnixStr
	commit = "commit-hash"
	branch = "commit-branch"
	buildHost = "build-host"
	buildUser = "build-user"
	doInit()
}

func TestNilString(t *testing.T) {
	reset()
	expected := strings.TrimSpace(`
Build product: -
Build time:   -
Build branch: -
Build commit: -
Build host:   -
Build user:   -
`)
	result := strings.TrimSpace(String())
	if result != expected {
		t.Fatal("Incorrect string returned:\n", result)
	}
}

func TestNilMap(t *testing.T) {
	reset()
	expected := map[string]string{
		"product":         "",
		"build_time":      "",
		"build_time_unix": "",
		"branch":          "",
		"commit":          "",
		"build_host":      "",
		"build_user":      "",
	}
	result := Map()
	if !reflect.DeepEqual(expected, result) {
		t.Fatalf("Incorrect map returned %#v", result)
	}
}

func TestSetStrings(t *testing.T) {
	setValues()
	expected := strings.TrimSpace(`
Build product: my-product
Build time:   ` + fmtTestTime + `
Build branch: commit-branch
Build commit: commit-hash
Build host:   build-host
Build user:   build-user
`)

	result := strings.TrimSpace(String())
	if result != expected {
		t.Fatalf("Incorrect string returned: expected=\n%s\nactual=%s\n", expected, result)
	}
}

func TestSetKVStrings(t *testing.T) {
	setValues()
	result := KVString()
	expected := `branch="commit-branch" build_host="build-host" build_time="` + fmtTestTime + `" build_time_unix="1459473825" build_user="build-user" commit="commit-hash" product="my-product"`
	if result != expected {
		t.Fatalf("expected:\n%s\nactual:\n%s\n", expected, result)
	}
}

func TestSetMap(t *testing.T) {
	setValues()
	expected := map[string]string{
		"product":         "my-product",
		"build_time":      fmtTestTime,
		"build_time_unix": testTimeUnixStr,
		"branch":          "commit-branch",
		"commit":          "commit-hash",
		"build_host":      "build-host",
		"build_user":      "build-user",
	}
	result := Map()
	if !reflect.DeepEqual(expected, result) {
		t.Fatalf("Incorrect map returned %#v", result)
	}
}

func TestJSON(t *testing.T) {
	setValues()
	expected := `{
 "product": "my-product",
 "build_time_unix": "1459473825",
 "build_time": "` + fmtTestTime + `",
 "commit": "commit-hash",
 "branch": "commit-branch",
 "build_host": "build-host",
 "build_user": "build-user"
}`

	result, _ := json.MarshalIndent(Info(), "", " ")
	if string(result) != expected {
		fmt.Println(string(result))
		t.Fatalf("Incorrect string returned: expected=\n%s\nactual=%s\n", expected, string(result))
	}

}

func TestForceReset(t *testing.T) {
	setValues()
	ForceReset(BuildInfo{
		Product: "overriden",
	})

	result := Info()
	if result.Product != "overriden" || result.Commit != "" {
		t.Fatal("Info was not reset", result)
	}
}
