// Copyright 2016 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package apirouter

import (
	"net/http"
	"reflect"
	"testing"
)

func TestParseDeviceAgentOK(t *testing.T) {
	agent, err := ParseDeviceAgent("overdrive 1.3.0.2503.160503.1646.d.2c39eb2 1.3 ios iPod touch 7.1.1")
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	expected := DeviceAgent{
		Product:        "overdrive",
		BuildVersion:   "1.3.0.2503.160503.1646.d.2c39eb2",
		Version:        "1.3",
		DeviceOS:       "ios",
		DeviceTypeName: "iPod touch 7.1.1",
	}
	if !reflect.DeepEqual(agent, expected) {
		t.Errorf("expected=%#v actual=%#v", expected, agent)
	}
}

func TestParseOtherAgent(t *testing.T) {
	_, err := ParseDeviceAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.94 Safari/537.36")
	if err == nil {
		t.Fatal("did not receive expected error")
	}
}

var normalizeTests = []struct {
	input    string
	expected string
}{
	{"", "0000.0000.0000.0000"},
	{"12", "0012.0000.0000.0000"},
	{"2.3", "0002.0003.0000.0000"},
	{"2.3.0", "0002.0003.0000.0000"},
	{"2.3.4", "0002.0003.0004.0000"},
	{"2.3.4.5", "0002.0003.0004.0005"},
	{"2.3.4.5.6", "0002.0003.0004.0005"},
	{"1.23.456.7891", "0001.0023.0456.7891"},
	{"1.1e", "0001.001e.0000.0000"},
	{"1.23.45-alpha", "0001.0023.45-alpha.0000"},
}

func TestAgentNormalizeVersion(t *testing.T) {
	for _, test := range normalizeTests {
		agent := DeviceAgent{Version: test.input}
		result := agent.NormalizeVersion()
		if result != test.expected {
			t.Errorf("input=%q expected=%q actual=%q", test.input, test.expected, result)
		}
	}
}

var versionGTETests = []struct {
	left  string
	right string
}{
	{"1", "2"},
	{"1.1", "2"},
	{"1.10", "1.11"},
	{"1.10", "1.10.1"},
	{"1.10", "1.10.0.1"},
}

func TestAgentVersionGTE(t *testing.T) {
	gte := func(left, right DeviceAgent) bool {
		result, err := left.VersionGTE(right)
		if err != nil {
			t.Errorf("Unexpected error for left=%s right=%s err=%s", left.Version, right.Version, err)
		}
		return result
	}

	for _, test := range versionGTETests {
		left := DeviceAgent{Version: test.left}
		right := DeviceAgent{Version: test.right}
		if gte(left, right) {
			t.Errorf("Returned true for %s >= %s", test.left, test.right)
		}
		if !gte(right, left) {
			t.Errorf("Returned false for %s >= %s", test.right, test.left)
		}
		if !gte(left, left) {
			t.Errorf("Returned false for %s >= %s", test.left, test.left)
		}
		if !gte(right, right) {
			t.Errorf("Returned false for %s >= %s", test.right, test.right)
		}
	}
}

func TestDeviceAgentForRequestOK(t *testing.T) {
	r, _ := http.NewRequest("GET", "/", nil)
	r.Header.Set("User-Agent", "overdrive 1.3.0.2503.160503.1646.d.2c39eb2 1.3 ios iPod touch 7.1.1")
	result := DeviceAgentForRequest(r)
	if result == nil {
		t.Fatal("Nil result")
	}
	if result.Version != "1.3" {
		t.Errorf("Incorrect version expected=%q actual=%q", "1.3", result.Version)
	}

	// check for cache hit by corrupting the header
	r.Header.Set("User-Agent", "should-not-reparse")
	result = DeviceAgentForRequest(r)
	if result == nil {
		t.Fatal("Nil result; expected cache hit")
	}
	if result.Version != "1.3" {
		t.Errorf("Incorrect version for cached result expected=%q actual=%q", "1.3", result.Version)
	}
}

func TestDeviceAgentForRequestErrors(t *testing.T) {
	r, _ := http.NewRequest("GET", "/", nil)
	// don't set the user agent
	result := DeviceAgentForRequest(r)
	if result != nil {
		t.Fatal("Unexpected result", result)
	}

	// Set to a valid string and check we still get a nil result due to cache hit
	r.Header.Set("User-Agent", "overdrive 1.3.0.2503.160503.1646.d.2c39eb2 1.3 ios iPod touch 7.1.1")
	result = DeviceAgentForRequest(r)
	if result != nil {
		t.Fatal("Unexpected result", result)
	}

	// Set to an invalid string
	r, _ = http.NewRequest("GET", "/", nil)
	r.Header.Set("User-Agent", "overdrive invalid")
	if result != nil {
		t.Fatal("Unexpected result", result)
	}
}
