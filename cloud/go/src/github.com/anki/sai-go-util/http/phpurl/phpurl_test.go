package phpurl

import (
	"net/url"
	"reflect"
	"testing"
)

var encodeTests = []struct {
	name        string
	input       interface{}
	vExpected   url.Values
	encExpected string
}{
	{
		name:        "simple array",
		input:       Arr{"num1", 123, 123.4, nil}, // nil value should be elided
		vExpected:   url.Values{"0": {"num1"}, "1": {"123"}, "2": {"123.4"}},
		encExpected: "0=num1&1=123&2=123.4",
	},
	{
		name: "complex map",
		input: Map{
			"key0": nil, // should be elided
			"key1": "value1",
			"key2": Map{
				"key2-1": "value2-1",
				"key3-1": Map{
					"key4-1-1": "value4-1-1",
					"key4-1-2": 42.3,
				},
				"key5-1": Arr{"value5-1-1", "value5-1-2", "value5-1-3"},
			},
		},
		vExpected: url.Values{
			"key2[key2-1]":           {"value2-1"},
			"key2[key3-1][key4-1-1]": {"value4-1-1"},
			"key2[key3-1][key4-1-2]": {"42.3"},
			"key2[key5-1][0]":        {"value5-1-1"},
			"key2[key5-1][1]":        {"value5-1-2"},
			"key2[key5-1][2]":        {"value5-1-3"},
			"key1":                   {"value1"},
		},
		encExpected: "key1=value1&key2%5Bkey2-1%5D=value2-1&key2%5Bkey3-1%5D%5Bkey4-1-1%5D=value4-1-1&key2%5Bkey3-1%5D%5Bkey4-1-2%5D=42.3&key2%5Bkey5-1%5D%5B0%5D=value5-1-1&key2%5Bkey5-1%5D%5B1%5D=value5-1-2&key2%5Bkey5-1%5D%5B2%5D=value5-1-3",
	},
}

func TestEncode(t *testing.T) {
	for _, test := range encodeTests {
		var values url.Values

		switch v := test.input.(type) {
		case Map:
			values = v.Values()
		case Arr:
			values = v.Values()
		default:
			t.Fatal("Unknown input type for test")
		}

		encoded := values.Encode()
		if !reflect.DeepEqual(values, test.vExpected) {
			t.Errorf("test=%q values mismatch", test.name)
		}

		if encoded != test.encExpected {
			t.Errorf("test=%q encoded mismatch\nexpected=%s\nactual=%s\n", test.name, test.encExpected, encoded)
		}
	}
}
