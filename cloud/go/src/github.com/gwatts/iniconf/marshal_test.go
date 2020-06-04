package iniconf

import (
	"reflect"
	"strings"
	"testing"
)

var rsIni = `
[section one]
key1 = val1
key2 = val2
bool1 = true
bool2 = true
int1 = 123
int2 = 456
subkey = subkey value
iface = iface value
`

type Foo string

type rsType struct {
	Key1     *string     `iniconf:"key1"`
	Key2     Foo         `iniconf:"key2"`
	Bool1    bool        `iniconf:"bool1"`
	Bool2    *bool       `iniconf:"bool2"`
	Int1     int64       `iniconf:"int1"`
	Int2     *int64      `iniconf:"int2"`
	Iface    interface{} `iniconf:"iface"`
	Sub      *rsSub
	ignored  *string
	NilPtr   *string     `iniconf:"nil1"`
	NilIface interface{} `iniconf:"nil1"`
}

type rsSub struct {
	SubKey  string `iniconf:"subkey"`
	ignored *rsType
}

func TestReadSection(t *testing.T) {
	var target rsType
	cfg, err := LoadString(rsIni, false)
	if err != nil {
		t.Fatal("Failed to parse data", err)
	}
	if err := cfg.ReadSection("section one", &target); err != nil {
		t.Fatal("Unexpected error", err)
	}
	expected := rsType{
		Key1:   pstr("val1"),
		Key2:   Foo("val2"),
		Bool1:  true,
		Bool2:  pbool(true),
		Int1:   123,
		Int2:   pint64(456),
		Sub:    &rsSub{SubKey: "subkey value"},
		Iface:  "iface value",
		NilPtr: pstr(""),
	}
	if !reflect.DeepEqual(target, expected) {
		t.Fatal("Did not match expected")
	}
}

func TestWriteIntoSection(t *testing.T) {
	cfg := New(false)
	i2 := int64(456)
	s1 := "s1"
	b2 := true
	v := &rsType{
		Key1:  &s1,
		Key2:  "s2",
		Bool1: true,
		Bool2: &b2,
		Int1:  123,
		Int2:  &i2,
		Sub: &rsSub{
			SubKey: "sub-value",
		},
		Iface: "iface-value",
	}
	v.Sub.ignored = v
	if err := cfg.UpdateSection("test-section", v); err != nil {
		t.Fatal("Unexpected error", err)
	}
	expected := strings.TrimSpace(`
[test-section]
key1 = s1
key2 = s2
bool1 = true
bool2 = true
int1 = 123
int2 = 456
iface = iface-value
subkey = sub-value`)

	result := strings.TrimSpace(cfg.String())
	if result != expected {
		t.Fatal("Result does not match expected")
	}
}

func pstr(s string) *string { return &s }
func pbool(b bool) *bool    { return &b }
func pint64(i int64) *int64 { return &i }
