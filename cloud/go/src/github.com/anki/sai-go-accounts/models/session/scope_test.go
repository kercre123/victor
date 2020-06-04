// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package session

import (
	"encoding/json"
	"strings"
	"testing"
)

func TestScopeContains(t *testing.T) {
	sm := NewScopeMask(UserScope, AdminScope)
	if sm.Contains(SupportScope) {
		t.Errorf("Incorrect match on SupportScope")
	}
	if !sm.Contains(AdminScope) {
		t.Errorf("Did not report match on AdminScope")
	}
	if !sm.Contains(UserScope) {
		t.Errorf("Did not report match on UserScope")
	}
}

var scopeStringTests = []struct {
	mask     ScopeMask
	expected string
}{
	{0, ""},
	{NewScopeMask(SupportScope), "support"},
	{NewScopeMask(SupportScope, AdminScope), "admin,support"},
}

func TestScopeMaskString(t *testing.T) {
	for _, test := range scopeStringTests {
		result := test.mask.String()
		if result != test.expected {
			t.Errorf("mismatch for sm=%d expected=%q actual=%q", uint32(test.mask), test.expected, result)
		}
	}
}

func TestScopeMarshaler(t *testing.T) {
	data := map[string]interface{}{"test": UserScope}
	result, err := json.Marshal(data)
	if err != nil {
		t.Fatal("json encode failed", err)
	}
	expected := `{"test":"user"}`
	if string(result) != expected {
		t.Errorf("expected=%q actual=%q", expected, string(result))
	}
}

func TestScopeUnmarshaler(t *testing.T) {
	var result struct{ Test Scope }
	input := `{"Test": "user"}`
	if err := json.Unmarshal([]byte(input), &result); err != nil {
		t.Fatal("Unmarshal failed", err)
	}
	if result.Test != UserScope {
		t.Errorf("expected=%v actual=%v", UserScope, result.Test)
	}
}

func TestScopeUnmarshalerBadScope(t *testing.T) {
	var result struct{ Test Scope }
	input := `{"Test": "invalid"}`
	err := json.Unmarshal([]byte(input), &result)
	if err == nil {
		t.Fatal("Unmarshal didn't return error")
	}
	if !strings.HasPrefix(err.Error(), "Unknown scope name") {
		t.Error("Didn't get expected error", err)
	}
}

var parseTests = []struct {
	input         string
	expectedMask  ScopeMask
	expectedError bool
}{
	{"user", NewScopeMask(UserScope), false},
	{"user,admin", NewScopeMask(UserScope, AdminScope), false},
	{"invalid,admin", ScopeMask(0), true},
	{"", ScopeMask(0), false},
}

func TestParseScopeMask(t *testing.T) {
	for _, test := range parseTests {
		result, err := ParseScopeMask(test.input)
		if test.expectedError {
			if err == nil {
				t.Errorf("DId not get expected error for input=%q", test.input)
			}
			continue
		}
		if result != test.expectedMask {
			t.Errorf("Did not get expected scope for input=%q expected=%#v actual=%#v",
				test.input, test.expectedMask, result)
		}
	}
}
