// Copyright Splunk, Inc. 2014
//
// Author: Gareth Watts <gareth@splunk.com>

package iniconf

import (
	"bufio"
	"bytes"
	"fmt"
	"io/ioutil"
	"os"
	"reflect"
	"strings"
	"testing"
)

var _ = fmt.Println

func TestNew(t *testing.T) {
	New(true)
}

func TestNewFile(t *testing.T) {
	i := NewFile("filename", true)
	if i.filename != "filename" {
		t.Errorf("Failed to set filename result=%s", i.filename)
	}
}

func TestNewSection(t *testing.T) {
	i := New(true)
	err := i.NewSection("section")
	if err != nil {
		t.Fatalf("Got err creating section err=%s", err)
	}
	if !i.HasSection("section") {
		t.Fatalf("HasSection returned false")
	}
	err = i.SetEntry("section", "key", "value")
	if err != nil {
		t.Errorf("Got err from SetEntry err=%s", err)
	}
	v, err := i.EntryString("section", "key")
	if err != nil {
		t.Fatalf("Got err while fetching key err=%s", err)
	}
	if v != "value" {
		t.Errorf("Expected v=\"value\" actual=%q", v)
	}
}

func TestAutoCreateSection(t *testing.T) {
	i := New(true)
	err := i.SetEntry("section1", "key1", "val1")
	if err != nil {
		t.Fatalf("SetEntry returned err=%s", err)
	}
	if !i.HasSection("section1") {
		t.Errorf("HasSection returned false")
	}
	result, err := i.EntryString("section1", "key1")
	if err != nil {
		t.Fatalf("GetEntry returned err=%s", err)
	}
	if result != "val1" {
		t.Errorf("GetEntry returned result=%s", result)
	}
}

func TestDeleteSection(t *testing.T) {
	i := New(true)
	i.NewSection("section1")
	i.SetEntry("section1", "key1", "val1")
	i.NewSection("section2")
	err := i.DeleteSection("section1")
	if err != nil {
		t.Errorf("Failed to delete section err=%s", err)
	}
	if i.HasSection("section1") {
		t.Error("section1 still retrievable")
	}

	if !i.HasSection("section2") {
		t.Error("section2 disappeared")
	}

	err = i.DeleteSection("section1")
	if err != ErrSectionNotFound {
		t.Errorf("Delete failed to return error - got err=%s", err)
	}
}

func TestSections(t *testing.T) {
	i := New(true)
	i.NewSection("section1")
	i.NewSection("section2")
	i.NewSection("section3")
	expected := []string{"section1", "section2", "section3"}
	result := i.Sections()
	if len(expected) != len(result) {
		t.Fatalf("Mismatch in section count expected=3 actual=%d", len(result))
	}
	for i := range expected {
		if result[i] != expected[i] {
			t.Errorf("Mismatch at i=%d", i)
		}
	}
}

func TestSectionEntryNames(t *testing.T) {
	i := New(true)
	i.NewSection("section")
	i.SetEntry("section", "k1", "v1")
	i.SetEntry("section", "k2", "v2")

	expected := []string{"k1", "k2"}
	result := i.EntryNames("section")
	if !reflect.DeepEqual(expected, result) {
		t.Errorf("EntryNames expected=%#v\nactual=%#v", expected, result)
	}
}

func TestSetEntryBadName(t *testing.T) {
	i := New(true)
	if err := i.SetEntry("section", "k1", "v1"); err != nil {
		t.Errorf("Unexpected error err=%v", err)
	}
}

func TestEmptySectionEmptyNames(t *testing.T) {
	i := New(true)
	if result := i.EntryNames("section"); result != nil {
		t.Errorf("expected=nil actual=%v", result)
	}
}

func TestDeleteEntry(t *testing.T) {
	i := New(true)
	i.NewSection("section1")
	i.SetEntry("section1", "key1", "val1")
	i.SetEntry("section1", "key2", "val2")
	err := i.DeleteEntry("section1", "key1")
	if err != nil {
		t.Errorf("Got error deleting entry err=%s", err)
	}

	_, err = i.EntryString("section1", "key1")
	if err != ErrEntryNotFound {
		t.Errorf("key still retrievable")
	}

	_, err = i.EntryString("section1", "key2")
	if err != nil {
		t.Errorf("key2 disappeared")
	}

	err = i.DeleteEntry("section1", "key1")
	if err != ErrEntryNotFound {
		t.Errorf("Delete failed to return err - err=%s", err)
	}

	err = i.DeleteEntry("sectionbad", "key1")
	if err != ErrEntryNotFound {
		t.Errorf("Delete failed to return err - err=%s", err)
	}
}

func TestInvalidEntryName(t *testing.T) {
	i := New(true)
	i.NewSection("section1")
	err := i.SetEntry("section1", "", "val")
	if err != ErrInvalidEntryName {
		t.Errorf("Expected ErrInvalidEntryName err=%s", err)
	}
}

func TestDuplicateSection(t *testing.T) {
	i := New(true)
	i.NewSection("section1")
	err := i.NewSection("section1")
	if err != ErrDuplicateSection {
		t.Errorf("Unexpected err=%s", err)
	}
}

func TestCaseSensitive(t *testing.T) {
	i := New(true)
	i.NewSection("Section")
	i.SetEntry("Section", "Key", "Value")
	_, err := i.EntryString("Section", "Key")
	if err != nil {
		t.Fatal("Failed to fetch Key")
	}
	if i.HasSection("section") {
		t.Fatal("Fetched section despite case sensitivity")
	}
	if _, err := i.EntryString("Section", "key"); err == nil {
		t.Fatal("Fetched key despite case sensitivity")
	}
}

func TestCaseInSensitive(t *testing.T) {
	i := New(false)
	i.NewSection("Section")
	i.SetEntry("Section", "Key", "Value")
	if !i.HasSection("Section") {
		t.Fatal("Failed to fetch Section")
	}
	if !i.HasSection("section") {
		t.Fatal("Failed to fetch section")
	}
	if _, err := i.EntryString("Section", "Key"); err != nil {
		t.Fatal("Failed to fetch Section:Key")
	}
	if _, err := i.EntryString("Section", "key"); err != nil {
		t.Fatal("Failed to fetch Section:key")
	}
	if _, err := i.EntryString("section", "key"); err != nil {
		t.Fatal("Failed to fetch section:key")
	}
}

type iniSection struct {
	sectionName string
	keyvals     map[string]string
}

// Check that a conf file exactly matches a list of sections/key/vals
func checkIniSections(t *testing.T, i *IniConf, checkSections []iniSection) {
	for _, test := range checkSections {
		if !i.HasSection(test.sectionName) {
			t.Errorf("Failed to fetch section %q", test.sectionName)
			continue
		}
		for key, val := range test.keyvals {
			entry, err := i.EntryString(test.sectionName, key)
			if err != nil {
				t.Errorf("Failed to fetch entry for section=%q key=%q err=%s", test.sectionName, key, err)
				continue
			}
			if val != entry {
				t.Errorf("Key mismatch for section=%q key=%q expected=%q actual=%q", test.sectionName, key, val, entry)
			}
		}
	}
}

var testIni1 = `
; test file
[section one]
key1 = val1
key two = line one
  line two
  line three

[section two]
key3 = val3
`
var testIniExpected = []iniSection{
	{"section one", map[string]string{"key1": "val1", "key two": "line one\nline two\nline three"}},
}

func TestLoadReader(t *testing.T) {
	br := bufio.NewReader(strings.NewReader(testIni1))
	i, err := LoadReader(br, false)
	if err != nil {
		t.Fatalf("Failed to read config: %s", err)
	}

	checkIniSections(t, i, testIniExpected)
}

func TestLoadString(t *testing.T) {
	i, err := LoadString(testIni1, false)
	if err != nil {
		t.Fatalf("Failed to read config: %s", err)
	}
	checkIniSections(t, i, testIniExpected)
}

func TestLoadFile(t *testing.T) {
	tf, err := ioutil.TempFile(os.TempDir(), "iniconf")
	if err != nil {
		t.Fatalf("Failed to create tempfile %s", err)
	}
	defer os.Remove(tf.Name())
	tf.Write([]byte("[section 1]\nkey1=val1\n"))
	tf.Close()

	i, err := LoadFile(tf.Name(), true)
	if err != nil {
		t.Fatalf("Failed to load conf file: %s", err)
	}
	v, _ := i.EntryString("section 1", "key1")
	if v != "val1" {
		t.Errorf("Failed to fetch key1")
	}
}

func TestLoadBadFilename(t *testing.T) {
	_, err := LoadFile("/invalid_filename", true)
	if err == nil {
		t.Errorf("Expected error err=%s", err)
	}
}

func TestLoadFileParseError(t *testing.T) {
	tf, err := ioutil.TempFile(os.TempDir(), "iniconf")
	if err != nil {
		t.Fatalf("Failed to create tempfile %s", err)
	}
	defer os.Remove(tf.Name())
	tf.Write([]byte("[section 1]\nkey1val1\n"))
	tf.Close()

	_, err = LoadFile(tf.Name(), true)
	if err == nil {
		t.Fatal("Expected error from Load")
	}
}

func TestLoadParseError(t *testing.T) {
	_, err := LoadString("[section1]\nkeyval1\n", false)
	if err == nil {
		t.Fatal("Expected load to return error")
	}
	if err.(*ParseError).code != ErrInvalidEntry {
		t.Fatalf("Unexpected error type %s", err)
	}
}

func TestLoadBadKVError(t *testing.T) {
	_, err := LoadString("[section1]\n=val1\n", false)
	if err != ErrInvalidEntryName {
		t.Fatalf("Expected err=ErrInvalidEntryName, actual=%v", err)
	}
}

func TestKeyOutsideSection(t *testing.T) {
	_, err := LoadString("key=val1\n", false)
	if err == nil {
		t.Fatal("Expected load to return error")
	}
	if err.(*ParseError).code != ErrNoSection {
		t.Fatalf("Unexpected error type %s", err)
	}
}

var envVals = map[string]string{
	"ini__section one__key one":      "value one",
	"otherini__section one__key two": "value two",
	"ini__section two":               "value three",
	"ini__section two__":             "value four",
	"ini____invalid section":         "value five",
	"ini__section three__key one":    "value six",
	"ini__section three__key two":    "value seven",
}
var envExpectedSections = map[string]map[string]string{
	"section one":   {"key one": "value one"},
	"section three": {"key one": "value six", "key two": "value seven"},
}

func TestLoadEnviron(t *testing.T) {
	for k, v := range envVals {
		os.Setenv(k, v)
	}
	// XXX Add defer to remove env vars when Go has an Unsetsenv() function added

	i, err := LoadEnviron("ini", false)
	if err != nil {
		t.Fatalf("Failed to load from environ: %s", err)
	}

	sections := i.Sections()
	if len(sections) != len(envExpectedSections) {
		t.Fatalf("Wrong section count expected=%d actual=%d sections=%#v", len(envExpectedSections), len(sections), sections)
	}
	for s, keyvals := range envExpectedSections {
		if !i.HasSection(s) {
			t.Errorf("Section %q is missing", s)
		}
		names := i.EntryNames(s)
		if len(names) != len(keyvals) {
			t.Errorf("Wrong number of entries for section=%q expected=%d actual=%d names=%#v", s, len(keyvals), len(names), names)
		}
		for key, val := range keyvals {
			eval, _ := i.EntryString(s, key)
			if eval != val {
				t.Errorf("Value mismatch for section=%q key=%q expected=%q actual=%q", s, key, val, eval)
			}
		}
	}
}

var nameTests = []struct {
	input  string
	output string
	ok     bool
}{
	{"a", "a", true},
	{"", "", false},
	{"  foobar ", "foobar", true},
	{"  ", "", false},
}

func TestValidateName(t *testing.T) {
	for _, test := range nameTests {
		output, ok := validateName(test.input)
		if ok != test.ok {
			t.Errorf("ok mismatch for input=%s expected=%t actual=%t", test.input, test.ok, ok)
		}
		if output != test.output {
			t.Errorf("output mismatch for input=%s expected=%s actual=%s", test.input, test.output, output)
		}
	}
}

var reloadBase = ` 
[section1]
key1 = val1
key2 = val2

[section2]
key3 = val3

[section3]
key4 = val4
`
var reloadNew = ` 
[section3]
key4 = val4-new
key5 = val5-new

[section1]
key2 = val2

[section4]
key6 = val6-new
`
var reloadExpected = []iniSection{
	{"section1", map[string]string{"key2": "val2"}},
	{"section3", map[string]string{"key4": "val4-new", "key5": "val5-new"}},
	{"section4", map[string]string{"key6": "val6-new"}},
}

func TestReload(t *testing.T) {
	tf, err := ioutil.TempFile(os.TempDir(), "iniconf")
	if err != nil {
		t.Fatalf("Failed to create tempfile %s", err)
	}
	defer os.Remove(tf.Name())
	tf.Write([]byte(reloadBase))

	i, err := LoadFile(tf.Name(), true)
	if err != nil {
		t.Fatalf("Failed to load conf file: %s", err)
	}

	tf.Truncate(0)
	tf.Seek(0, 0)
	tf.Write([]byte(reloadNew))

	err = i.Reload()
	if err != nil {
		t.Fatalf("Reload failed err=%s", err)
	}
	checkIniSections(t, i, reloadExpected)
}

func TestReloadNoFilename(t *testing.T) {
	i := New(true)
	err := i.Reload()
	if err != ErrReloadNoFilename {
		t.Errorf("Expected ErrReloadNoFilename err=%s", err)
	}
}

func TestReloadNoFile(t *testing.T) {
	tf, err := ioutil.TempFile(os.TempDir(), "iniconf")
	if err != nil {
		t.Fatalf("Failed to create tempfile %s", err)
	}
	defer os.Remove(tf.Name())
	tf.Write([]byte(reloadBase))

	i, err := LoadFile(tf.Name(), true)
	if err != nil {
		t.Fatalf("Failed to load conf file: %s", err)
	}

	os.Remove(tf.Name())
	err = i.Reload()
	if err == nil {
		t.Errorf("Expected error from reload err=nill")
	}
}

type testObv struct {
	newSections  []string
	delSections  []string
	entryChanges map[string][]entryChange
}

func (o *testObv) IniSectionAdded(i *IniConf, sectionName string) {
	o.newSections = append(o.newSections, sectionName)
}

func (o *testObv) IniSectionDeleted(i *IniConf, sectionName string) {
	o.delSections = append(o.delSections, sectionName)
}

func (o *testObv) IniEntryChanged(i *IniConf, sectionName, entryName, oldValue, newValue string) {
	if o.entryChanges == nil {
		o.entryChanges = make(map[string][]entryChange)
	}
	o.entryChanges[sectionName] = append(o.entryChanges[sectionName],
		entryChange{sectionName, entryName, oldValue, newValue})
}

func TestReloadObservers(t *testing.T) {
	tf, err := ioutil.TempFile(os.TempDir(), "iniconf")
	if err != nil {
		t.Fatalf("Failed to create tempfile %s", err)
	}
	defer os.Remove(tf.Name())
	tf.Write([]byte(reloadBase))

	i, err := LoadFile(tf.Name(), true)
	if err != nil {
		t.Fatalf("Failed to load conf file: %s", err)
	}

	gobv := new(testObv)
	obv := new(testObv)
	i.RegisterGlobalObserver(gobv)
	i.RegisterSectionObserver("section1", obv)

	tf.Truncate(0)
	tf.Seek(0, 0)
	tf.Write([]byte(reloadNew))

	err = i.Reload()
	if err != nil {
		t.Fatalf("Reload failed err=%s", err)
	}

	expectedNewSections := []string{"section4"}
	expectedDelSections := []string{"section2"}
	expectedEntryChanges := map[string][]entryChange{
		"section1": {
			entryChange{"section1", "key1", "val1", ""},
		},
		"section2": {
			entryChange{"section2", "key3", "val3", ""},
		},
		"section3": {
			entryChange{"section3", "key4", "val4", "val4-new"},
			entryChange{"section3", "key5", "", "val5-new"},
		},
		"section4": {
			entryChange{"section4", "key6", "", "val6-new"},
		},
	}
	if !reflect.DeepEqual(gobv.newSections, expectedNewSections) {
		t.Errorf("Observer newSections mismatch")
	}
	if !reflect.DeepEqual(gobv.delSections, expectedDelSections) {
		t.Errorf("Observer delSections mismatch")
	}
	if !reflect.DeepEqual(gobv.entryChanges, expectedEntryChanges) {
		t.Errorf("Observer entryChanges mismatch:\nexpected=%#v\nactual=%#v\n", expectedEntryChanges, gobv.entryChanges)
	}

	expectedEntryChanges = map[string][]entryChange{
		"section1": {
			entryChange{"section1", "key1", "val1", ""},
		},
	}
	if !reflect.DeepEqual(obv.entryChanges, expectedEntryChanges) {
		t.Errorf("Section observer entryChanges mismatch:\nexpected=%#v\nactual=%#v\n", expectedEntryChanges, gobv.entryChanges)
	}
}

type testNewSectionObv struct {
	newSections []string
}

func (o *testNewSectionObv) IniSectionAdded(i *IniConf, sectionName string) {
	o.newSections = append(o.newSections, sectionName)
}

type testDelSectionObv struct {
	delSections []string
}

func (o *testDelSectionObv) IniSectionDeleted(i *IniConf, sectionName string) {
	o.delSections = append(o.delSections, sectionName)
}

type testEntryChangedObv struct {
	entryChanges map[string][]entryChange
}

func (o *testEntryChangedObv) IniEntryChanged(i *IniConf, sectionName, entryName, oldValue, newValue string) {
	if o.entryChanges == nil {
		o.entryChanges = make(map[string][]entryChange)
	}
	o.entryChanges[sectionName] = append(o.entryChanges[sectionName],
		entryChange{sectionName, entryName, oldValue, newValue})
}

func TestNewSectionObserver(t *testing.T) {
	i := New(true)
	gobv := new(testNewSectionObv)
	obv := new(testNewSectionObv)
	i.RegisterGlobalObserver(gobv)
	i.RegisterSectionObserver("section2", obv)

	i.NewSection("section1")
	i.NewSection("section2")
	expected := []string{"section1", "section2"}
	if !reflect.DeepEqual(gobv.newSections, expected) {
		t.Errorf("global obv mismatch. expected=%#v actual=%#v", expected, gobv.newSections)
	}

	expected = []string{"section2"}
	if !reflect.DeepEqual(obv.newSections, expected) {
		t.Errorf("section obv mismatch. expected=%#v actual=%#v", expected, obv.newSections)
	}
}

func TestDeleteSectionObserver(t *testing.T) {
	i := New(true)
	gobv := new(testDelSectionObv)
	obv := new(testDelSectionObv)
	i.NewSection("section1")
	i.NewSection("section2")
	i.NewSection("section3")

	i.RegisterGlobalObserver(gobv)
	i.RegisterSectionObserver("section2", obv)

	i.DeleteSection("section2")
	i.DeleteSection("section3")
	i.DeleteSection("section4")

	expected := []string{"section2", "section3"}
	if !reflect.DeepEqual(gobv.delSections, expected) {
		t.Errorf("global obv mismatch. expected=%#v actual=%#v", expected, gobv.delSections)
	}

	expected = []string{"section2"}
	if !reflect.DeepEqual(obv.delSections, expected) {
		t.Errorf("section obv mismatch. expected=%#v actual=%#v", expected, obv.delSections)
	}
}

func TestEntryChangeObserver(t *testing.T) {
	i := New(true)
	gobv := new(testEntryChangedObv)
	obv := new(testEntryChangedObv)
	i.NewSection("section1")
	i.NewSection("section2")

	i.RegisterGlobalObserver(gobv)
	i.RegisterSectionObserver("section2", obv)

	i.SetEntry("section1", "s1-1", "v1")
	i.SetEntry("section1", "s1-2", "v2")
	i.SetEntry("section1", "s1-2", "v2-2")
	i.DeleteEntry("section1", "s1-2")

	i.SetEntry("section2", "s2-1", "v1")
	i.SetEntry("section2", "s2-2", "v2")
	i.DeleteEntry("section2", "s2-1")

	expected := map[string][]entryChange{
		"section1": {
			entryChange{"section1", "s1-1", "", "v1"},
			entryChange{"section1", "s1-2", "", "v2"},
			entryChange{"section1", "s1-2", "v2", "v2-2"},
			entryChange{"section1", "s1-2", "v2-2", ""},
		},
		"section2": {
			entryChange{"section2", "s2-1", "", "v1"},
			entryChange{"section2", "s2-2", "", "v2"},
			entryChange{"section2", "s2-1", "v1", ""},
		},
	}
	if !reflect.DeepEqual(gobv.entryChanges, expected) {
		t.Errorf("global obv mismatch.\nexpect=%#v\nactual=%#v", expected, gobv.entryChanges)
	}

	expected = map[string][]entryChange{
		"section2": {
			entryChange{"section2", "s2-1", "", "v1"},
			entryChange{"section2", "s2-2", "", "v2"},
			entryChange{"section2", "s2-1", "v1", ""},
		},
	}
	if !reflect.DeepEqual(obv.entryChanges, expected) {
		t.Errorf("section obv mismatch.\nexpect=%#v\nactual=%#v", expected, obv.entryChanges)
	}
}

func TestDisableObservers(t *testing.T) {
	i := New(true)
	gobv := new(testObv)
	obv := new(testObv)

	i.RegisterGlobalObserver(gobv)
	i.RegisterSectionObserver("section", obv)

	i.DisableObservers()
	i.NewSection("section1")

	i.EnableObservers()
	i.NewSection("section2")

	if len(gobv.newSections) != 1 || gobv.newSections[0] != "section2" {
		t.Errorf("Unexpected list of newsections actual=%v", gobv.newSections)
	}
}

func TestUnregisterObserver(t *testing.T) {
	i := New(true)
	gobv := new(testObv)
	obv := new(testObv)

	i.RegisterGlobalObserver(gobv)
	i.RegisterSectionObserver("section", obv)

	i.NewSection("section")

	i.UnregisterGlobalObserver(gobv)
	i.UnregisterSectionObserver("section", obv)

	i.DeleteSection("section")
	i.NewSection("section")

	if len(gobv.newSections) != 1 || len(gobv.delSections) != 0 {
		t.Errorf("Unregister failed for gobv")
	}

	if len(obv.newSections) != 1 || len(obv.delSections) != 0 {
		t.Errorf("Unregister failed for obv")
	}
}

func TestUnregisterUnknown(t *testing.T) {
	i := New(true)
	i.NewSection("section1")
	obv := new(testObv)

	i.UnregisterGlobalObserver(obv)
	i.UnregisterSectionObserver("section1", obv)
	i.UnregisterSectionObserver("section2", obv)
}

func catchPanicErr(expected error, f func()) (err error) {
	defer func() {
		if e := recover(); e != nil {
			err = e.(error)
			if err != expected {
				panic(e)
			}
		}
	}()
	f()
	return err
}

// trying to register anything except a supported interface should panic
func TestRegisterBadInterface(t *testing.T) {
	i := New(true)
	err := catchPanicErr(ErrBadObserver, func() { i.RegisterSectionObserver("section", 42) })
	if err != ErrBadObserver {
		t.Errorf("RegisterSetionObserver failed to panic")
	}

	err = catchPanicErr(ErrBadObserver, func() { i.RegisterGlobalObserver(42) })
	if err != ErrBadObserver {
		t.Errorf("RegisterGlobalObserver failed to panic")
	}
}

var evTests = []string{"", "   ", "\t", "\n\n"}

func TestAssignEmptyValue(t *testing.T) {
	i := New(true)
	i.NewSection("section")
	for _, test := range evTests {
		err := i.SetEntry("section", "key", test)
		if err != ErrEmptyEntryValue {
			t.Errorf("Didn't get error response for string=%q", test)
		}
	}
}

var newExpected = `[section1]
k1 = v1
k2 = v2
k3 = line one
    line two
    line three
k4 = line four
    line five
    line six

[section2]
k5 = v5
`

func TestSaveNew(t *testing.T) {
	tf, err := ioutil.TempFile(os.TempDir(), "iniconf")
	if err != nil {
		t.Fatalf("Failed to create tempfile %s", err)
	}
	defer os.Remove(tf.Name())

	i := New(true)
	i.NewSection("section1")
	i.SetEntry("section1", "k1", "v1")
	i.SetEntry("section1", "k2", "v2")
	i.SetEntry("section1", "k3", "line one\nline two\nline three")
	i.SetEntry("section1", "k4", "line four\n\nline five\nline six")

	i.NewSection("section2")
	i.SetEntry("section2", "k5", "v5")

	err = i.SaveToFilename(tf.Name())
	if err != nil {
		t.Errorf("Error during save err=%s", err)
	}

	result, _ := ioutil.ReadFile(tf.Name())
	if !bytes.Equal([]byte(newExpected), result) {
		t.Errorf("Unexpected result.  expected=\n%q\nactual=\n%q", newExpected, result)
	}
}

var strExpected = `[section1]
k1 = v1
k2 = v2
`

func TestToString(t *testing.T) {
	i := New(true)
	i.NewSection("section1")
	i.SetEntry("section1", "k1", "v1")
	i.SetEntry("section1", "k2", "v2")
	result := i.String()

	if result != strExpected {
		t.Errorf("Unexpected reuslt.  expected=\n%q\nactual=\n%q", strExpected, result)
	}
}

var updateStart = `
; header comment
; header comment line two

[section1]
k1 = v1
k2 = v2

[section2]
k3 = long
 entry
k4 = v4

[section3]
k5 = v5

[section5]
; contains only a comment

[section5.1]
; contains an opening comment; new entry should appear below

; and above the trailing comment

[section4]
; section 4 comment
k6 = v6

; trailer comment
`
var updateExpected = `; header comment
; header comment line two

[section1]
k1 = v1
k2 = v2-new

[section2]
k3 = long
 entry
k4-new = v4-new
k4-new2 = a long entry
    split across
    three lines

[section5]
; contains only a comment
k7 = v7

[section5.1]
; contains an opening comment; new entry should appear below
k7.1 = v7.1

; and above the trailing comment

[section4]
; section 4 comment
k6 = v6

; trailer comment

[section6]
k8 = v8
`

func TestUpdateExisting(t *testing.T) {
	tf, err := ioutil.TempFile(os.TempDir(), "iniconf")
	if err != nil {
		t.Fatalf("Failed to create tempfile %s", err)
	}
	defer os.Remove(tf.Name())
	tf.Write([]byte(updateStart))
	tf.Close()

	i, err := LoadFile(tf.Name(), true)
	if err != nil {
		t.Fatalf("Failed to load conf file: %s", err)
	}
	i.SetEntry("section1", "k2", "v2-new")

	i.DeleteEntry("section2", "k4")
	i.SetEntry("section2", "k4-new", "v4-new")
	i.SetEntry("section2", "k4-new2", "a long entry\nsplit across\nthree lines")
	i.SetEntry("section5", "k7", "v7")
	i.SetEntry("section5.1", "k7.1", "v7.1")

	i.DeleteSection("section3")

	i.NewSection("section6")
	i.SetEntry("section6", "k8", "v8")

	err = i.Save()
	if err != nil {
		t.Fatalf("Err during save err=%s", err)
	}

	result, _ := ioutil.ReadFile(tf.Name())
	if !bytes.Equal([]byte(updateExpected), result) {
		t.Errorf("Unexpected result.  expected=\n%s\nactual=\n%s", updateExpected, result)
	}
}

// Test updating a file when the on disk copy has been removed
// should just save it as new
func TestUpdateMissing(t *testing.T) {
	conf := "[section1]\nkey1 = val1\n"
	tf, err := ioutil.TempFile(os.TempDir(), "iniconf")
	if err != nil {
		t.Fatalf("Failed to create tempfile %s", err)
	}
	fmt.Println("fn", tf.Name())
	defer os.Remove(tf.Name())

	i, _ := LoadString(conf, false)
	i.SaveToFilename(tf.Name())
	os.Remove(tf.Name())

	if err := i.Save(); err != nil {
		t.Fatalf("Error while saving: %s", err)
	}

	result, _ := ioutil.ReadFile(tf.Name())
	if !bytes.Equal([]byte(conf), result) {
		t.Errorf("Unexpected result.  expected=\n%s\nactual=\n%s", conf, result)
	}
}

func TestUpdateParseError(t *testing.T) {
	conf := "[section1]\nkey1 = val1\n"
	tf, err := ioutil.TempFile(os.TempDir(), "iniconf")
	if err != nil {
		t.Fatalf("Failed to create tempfile %s", err)
	}
	fmt.Println("fn", tf.Name())
	defer os.Remove(tf.Name())

	i, _ := LoadString(conf, false)
	i.SaveToFilename(tf.Name())

	// Add a parse error to the on-disk copy
	tf.Seek(0, 2)
	tf.Write([]byte("\nparse error\n"))

	if err := i.Save().(*ParseError); err.Code() != ErrInvalidEntry {
		t.Fatalf("Expected error=%q actual=%q", ErrInvalidEntry, err)
	}
}

func TestSaveNoFilename(t *testing.T) {
	i, err := LoadString("[section1]\nk=v\n", false)
	if err != nil {
		t.Fatalf("Unexpected error: %s", err)
	}
	err = i.Save()
	if err != ErrSaveNoFilename {
		t.Errorf("Expected err=ErrSaveNoFilename, actual=%q", err)
	}
}
