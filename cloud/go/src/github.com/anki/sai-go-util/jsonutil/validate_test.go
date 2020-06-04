package jsonutil

import (
	"reflect"
	"sort"
	"testing"
)

var whitelistTests = []struct {
	name       string
	input      string
	WhiteField []WhiteField
	badFields  []string
}{
	{"simple1", `{"f1": "v1"}`, []WhiteField{{"f1", nil}}, nil},
	{"simple2", `{"f1": "v1"}`, []WhiteField{}, []string{"f1"}},
	{"simple3", `{"f1": "v1", "f2":2, "f3":3}`, []WhiteField{{"f2", nil}}, []string{"f1", "f3"}},
	{"simple4", `{"f1": "v1", "f2":2, "f3":3}`, []WhiteField{{"f2", nil}, {"f3", nil}}, []string{"f1"}},
	{"subobj1", `{"f1": "v1", "f2": {"n1": 1}}`, []WhiteField{{"f1", nil}}, []string{"f2"}},
	{"subobj1", `{"f1": "v1", "f2": {"n1": 1}}`, []WhiteField{
		{"f1", nil},
		{"f2", []WhiteField{{"n2", nil}}},
	}, []string{"f2.n1"}},
	{"subobj1", `{"f1": "v1", "f2": {"n1": 1}}`, []WhiteField{
		{"f1", nil},
		{"f2", []WhiteField{{"n1", nil}}},
	}, nil},
	{"subarr1", `{"f1": "v1", "f2": [1,2,3]}`, []WhiteField{{"f1", nil}, {"f2", nil}}, nil},
	{"subarr2", `{"f1": "v1", "f2": [{"n1": 0}]}`, []WhiteField{{"f1", nil}, {"f2", nil}}, nil},
	{"subarr3", `{"f1": "v1", "f2": [{"n1": 0}]}`, []WhiteField{
		{"f1", nil},
		{"f2", []WhiteField{{"n2", nil}}}}, []string{"f2.0.n1"}},
	{"subarr3", `{"f1": "v1", "f2": [{"n1": 0}, {"n2": 1}, {"n2": 1, "n3": 1}]}`, []WhiteField{
		{"f1", nil},
		{"f2", []WhiteField{{"n2", nil}}}}, []string{"f2.0.n1", "f2.2.n3"}},
}

func TestWhitelist(t *testing.T) {
	for _, test := range whitelistTests {
		_, err := ValidateKeys([]byte(test.input), test.WhiteField)
		if test.badFields == nil {
			if err != nil {
				t.Errorf("Unexpected errors test=%q input=%q err=%s", test.name, test.input, err)
			}
			continue
		}
		if badFields, ok := err.(BadFieldsError); ok {
			sort.Strings(test.badFields)
			sort.Strings(badFields)
			if !reflect.DeepEqual(test.badFields, badFields.Fields()) {
				t.Errorf("Bad field list doesn't match test=%q expected=%v actual=%#v", test.name, test.badFields, badFields.Fields())
			}
		} else {
			t.Errorf("Unexpected error test=%q err=%#v", test.name, err)
		}
	}
}

var validTests = []struct {
	name          string
	input         string
	WhiteField    []WhiteField
	expectedValid map[string]interface{}
}{
	{"simple1", `{"f1": "v1"}`, []WhiteField{{"f1", nil}}, map[string]interface{}{"f1": true}},
	{"simple2", `{"f1": "v1", "f2": 2}`, []WhiteField{{"f1", nil}, {"f2", nil}}, map[string]interface{}{"f1": true, "f2": true}},
	{"subobj1", `{"f1": "v1", "f2": {"n1": 1}}`, []WhiteField{{
		"f1", nil},
		{"f2", []WhiteField{{"n1", nil}}}},
		map[string]interface{}{"f1": true, "f2": map[string]interface{}{"n1": true}}},
	{"subarr1", `{"f1": "v1", "f2": [{"n1": 1}, {"n1": 2}]}`, []WhiteField{{
		"f1", nil},
		{"f2", []WhiteField{{"n1", nil}}}},
		map[string]interface{}{
			"f1": true,
			"f2": []interface{}{
				map[string]interface{}{"n1": true},
				map[string]interface{}{"n1": true},
			}}},
}

func TestValid(t *testing.T) {
	for _, test := range validTests {
		valid, err := ValidateKeys([]byte(test.input), test.WhiteField)
		if !reflect.DeepEqual(valid, test.expectedValid) {
			t.Errorf("valid mismatch for test=%q expected=%#v actual=%#v err=%v", test.name, test.expectedValid, valid, err)
		}
	}
}
