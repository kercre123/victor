// Copyright Splunk, Inc. 2014
//
// Author: Gareth Watts <gareth@splunk.com>

package iniconf

import (
	"strings"
	"testing"
)

var primaryConf = `
[section1]
key1 = primary1
key2 = primary2
`

var secondaryConf = `
[section1]
key2 = secondary2
key3 = secondary3

[section2]
key4 = secondary4

[bool]
b1 = true

[int]
i1 = 1234
`

var chainTests = []struct {
	section        string
	key            string
	expectedResult string
	expectedErr    error
}{
	{"section1", "key1", "primary1", nil},
	{"section1", "key2", "primary2", nil},
	{"section1", "key3", "secondary3", nil},
	{"section2", "key4", "secondary4", nil},
	{"section2", "key5", "", ErrEntryNotFound},
	{"section3", "key6", "", ErrEntryNotFound},
}

func runChainTests(t *testing.T, chain *ConfChain) {
	for _, test := range chainTests {
		result, err := chain.EntryString(test.section, test.key)
		if result != test.expectedResult {
			t.Errorf("section=%s key=%s expectedResult=%s actual=%s", test.section, test.key, test.expectedResult, result)
		}
		if err != test.expectedErr {
			t.Errorf("section=%s key=%s expectedErr=%s err=%s", test.section, test.key, test.expectedErr, err)
		}
	}
}

func TestChain(t *testing.T) {
	primary, _ := LoadReader(strings.NewReader(primaryConf), true)
	secondary, _ := LoadReader(strings.NewReader(secondaryConf), true)
	chain := NewConfChain(primary, secondary)
	runChainTests(t, chain)
}

func TestAddConf(t *testing.T) {
	primary, _ := LoadReader(strings.NewReader(primaryConf), true)
	secondary, _ := LoadReader(strings.NewReader(secondaryConf), true)
	chain := NewConfChain(primary)
	chain.AddConf(secondary)
	runChainTests(t, chain)
}

func TestInt(t *testing.T) {
	primary, _ := LoadReader(strings.NewReader(primaryConf), true)
	secondary, _ := LoadReader(strings.NewReader(secondaryConf), true)
	chain := NewConfChain(primary, secondary)

	result, err := chain.EntryInt("int", "i1")
	if result != 1234 {
		t.Errorf("Failed to fetch int expected=1234 actual=%d", result)
	}
	if err != nil {
		t.Errorf("int fetch returned err=%s", err)
	}
}

func TestBool(t *testing.T) {
	primary, _ := LoadReader(strings.NewReader(primaryConf), true)
	secondary, _ := LoadReader(strings.NewReader(secondaryConf), true)
	chain := NewConfChain(primary, secondary)

	result, err := chain.EntryBool("bool", "b1")
	if result != true {
		t.Errorf("Failed to fetch bool expected=true actual=%t", result)
	}
	if err != nil {
		t.Errorf("bool fetch returned err=%s", err)
	}
}

func TestEmptyChain(t *testing.T) {
	chain := NewConfChain()
	result, err := chain.EntryBool("bool", "b1")
	if result != false || err != ErrEmptyConfChain {
		t.Errorf("Didn't received expected error expected=ErrEmptyConfChain acutal=%v", err)
	}
}

var shouldPanicTests = []struct {
	name        string
	shouldPanic bool
	expected    interface{}
	f           func(c *ConfChain) interface{}
}{
	{"StringP", true, nil, func(c *ConfChain) interface{} { return c.EntryStringP("invalid", "invalid") }},
	{"StringP", false, "secondary2", func(c *ConfChain) interface{} { return c.EntryStringP("section1", "key2") }},
	{"IntP", true, nil, func(c *ConfChain) interface{} { return c.EntryIntP("invalid", "invalid") }},
	{"IntP", false, int64(1234), func(c *ConfChain) interface{} { return c.EntryIntP("int", "i1") }},
	{"BoolP", true, nil, func(c *ConfChain) interface{} { return c.EntryBoolP("invalid", "invalid") }},
	{"BoolP", false, true, func(c *ConfChain) interface{} { return c.EntryBoolP("bool", "b1") }},
}

func TestPanics(t *testing.T) {
	secondary, _ := LoadReader(strings.NewReader(secondaryConf), true)
	chain := NewConfChain(secondary)
	for _, test := range shouldPanicTests {
		var result interface{}
		didPanic := false
		func() {
			defer func() {
				err := recover()
				didPanic = err != nil
			}()
			result = test.f(chain)
		}()
		if didPanic != test.shouldPanic {
			t.Errorf("Panic mismatch for test=%s didPanic=%t shouldPanic=%t",
				test.name, didPanic, test.shouldPanic)
		}
		if !test.shouldPanic && result != test.expected {
			t.Errorf("Incorrect result value for test=%s expected=%#v actual=%#v",
				test.name, test.expected, result)
		}
	}
}

// Test that a bad value in the primary results in an error
// that's returned to the caller, rather than falling back
// to the secondary
// Only ErrEntryNotFound should fall through
func TestErrPassThru(t *testing.T) {
	primary, _ := LoadString("[int]\ni1 = invalid\n", true)
	secondary, _ := LoadReader(strings.NewReader(secondaryConf), true)
	chain := NewConfChain(primary, secondary)

	result, err := chain.EntryInt("int", "i1")
	if err == nil || result == 1234 {
		t.Fatalf("Unexpected result=%d err=%s", result, err)
	}

	if err == ErrEntryNotFound {
		t.Fatalf("Unexpected err=%s", err)
	}
}
