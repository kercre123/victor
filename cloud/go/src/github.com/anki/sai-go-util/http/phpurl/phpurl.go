// Package phpurl provides a convenient method of converting nested map and array
// structures to PHP-compatible url query values.
//
// eg Map{"key1": Map{"key2": Arr{"one", 2, 3}}}
// encodes to key1%5Bkey2%5D%5B0%5D=one&key1%5Bkey2%5D%5B1%5D=2&key1%5Bkey2%5D%5B2%5D=3
// (which is key1[key2][0]=one&key1[key2][1]=2&key1[key2][2]=3 with decoded brackets)
package phpurl

import (
	"fmt"
	"net/url"
	"strconv"
)

// Map holds a map of strings to values that can be encoded to a flat url-encoded
// namespace in PHP format.
//
// Values may included nested Map or Arr values; any other value will be converted
// to a string.
//
// Nil values are dropped to remain compatible with PHP's http_build_query().
type Map map[string]interface{}

func (m Map) values(nested bool) (values url.Values) {
	values = make(url.Values)
	for k, v := range m {
		values = addValue(values, nested, k, v)
	}
	return values
}

func (m Map) Values() url.Values {
	return m.values(false)
}

// Arr holds an array of values that can be encoded to a flat url-encoded
// namespace in PHP format.
//
// Values may included nested Map or Arr values; any other value will be
// converted  to a string.
//
// Nil values are dropped to remain compatible with PHP's http_build_query().
type Arr []interface{}

func (a Arr) values(nested bool) (values url.Values) {
	values = make(url.Values)
	for i, v := range a {
		values = addValue(values, nested, strconv.Itoa(i), v)
	}
	return values
}

func (a Arr) Values() url.Values {
	return a.values(false)
}

func addValue(values url.Values, nested bool, k string, v interface{}) url.Values {
	if nested {
		k = "[" + k + "]"
	}
	switch v := v.(type) {
	case Map:
		for k2, v2 := range v.values(true) {
			values[k+k2] = v2
		}

	case Arr:
		for k2, v2 := range v.values(true) {
			values[k+k2] = v2
		}

	case string:
		values[k] = append(values[k], v)

	default:
		if v != nil { // nil values are elided
			values[k] = append(values[k], fmt.Sprintf("%v", v))
		}
	}

	return values
}
