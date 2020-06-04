package jsonutil

// UnmarshaledMapDepth traverses a JSON object that has been unmarshaled with
// json.Unmarshal(), and calculates the maximum depth of the object.
//
// To understand the possible types used by json.Unmarshal(), see:
//   https://golang.org/pkg/encoding/json/#Unmarshal
//
// Examples:
//   `{}`                                   => 1
//   `{ "Field": "Value"}`                  => 1
//   `{ "Field": {}}`                       => 2
//   `{ "Field": { "SubField": [ 1, 3] } }` => 3
func UnmarshaledMapDepth(v map[string]interface{}) int {
	return 1 + mapDepth(v)
}

func mapDepth(m map[string]interface{}) int {
	for _, v := range m {
		switch tv := v.(type) {
		case map[string]interface{}:
			return 1 + mapDepth(tv)
		case []interface{}:
			return 1 + sliceDepth(tv)
		}
	}
	return 0
}

func sliceDepth(s []interface{}) int {
	for _, v := range s {
		switch tv := v.(type) {
		case map[string]interface{}:
			return 1 + mapDepth(tv)
		case []interface{}:
			return 1 + sliceDepth(tv)
		}
	}
	return 0
}
