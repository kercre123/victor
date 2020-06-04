package apiclient

import (
	"encoding/json"
	"fmt"
	autil "github.com/anki/sai-go-util"
	"github.com/stretchr/testify/assert"
	"testing"
)

var (
	jsonData = `
    {
      "location": {
        "cities": [ "Boom", "Brussels", "Aalst" ],
        "country": "Belgium",
        "landmarks" : [
          ["Tomorrowland"],
          ["Atomium", "Grand Place"],
          ["Belfry", "St. Martin's"]
        ],
        "coordinates": [
          { "latitude": 51.086, "longitude": 4.363 },
          { "latitude": 50.847, "longitude": 4.355 },
          { "latitude": 50.933, "longitude": 4.033 }
        ]
      }
    }
  `
	parsedJsonData Json
)

func init() {
	autil.Must1(json.Unmarshal([]byte(jsonData), &parsedJsonData))
}

// table tests
var fieldArrayTests = []struct {
	args []interface{}
	out  interface{}
	err  error
}{
	{args: []interface{}{"location", "country"}, out: "Belgium", err: nil},
	{args: []interface{}{"location", "cities", 0}, out: "Boom", err: nil},
	{args: []interface{}{"location", "coordinates", 1, "latitude"}, out: 50.847, err: nil},
	{args: []interface{}{"location", "landmarks", 1, 0}, out: "Atomium", err: nil},
	{args: []interface{}{"location", 0}, out: nil, err: NotArray},
	{args: []interface{}{"location", "cities", "first"}, out: nil, err: NotObject},
	{args: []interface{}{"location", 6.022}, out: nil, err: KeyWrongType},
	{args: []interface{}{"location", "coordinates", 42}, out: nil, err: OutOfBounds},
	{args: []interface{}{"location", "oblast", 0}, out: nil, err: NotFound},
	{args: []interface{}{0}, out: nil, err: NotArray},
}

func TestFieldArray(t *testing.T) {
	// run the tests
	for i, jt := range fieldArrayTests {
		out, err := parsedJsonData.FieldArray(jt.args...)
		assert.Equal(t, jt.err, err, fmt.Sprintf("test %d, error", i))
		assert.Equal(t, jt.out, out, fmt.Sprintf("test %d, output", i))
	}
}

// table tests
var fieldArrayStrTests = []struct {
	args []interface{}
	out  interface{}
	err  error
}{
	{args: []interface{}{"location", "country"}, out: "Belgium", err: nil},
	{args: []interface{}{"location", "cities", 0}, out: "Boom", err: nil},
	{args: []interface{}{"location", "coordinates", 1, "latitude"}, out: "", err: NotString},
	{args: []interface{}{"location", "landmarks", 1, 0}, out: "Atomium", err: nil},
	{args: []interface{}{"location", 0}, out: "", err: NotArray},
	{args: []interface{}{"location", "cities", "first"}, out: "", err: NotObject},
	{args: []interface{}{"location", 6.022}, out: "", err: KeyWrongType},
	{args: []interface{}{"location", "coordinates", 42}, out: "", err: OutOfBounds},
	{args: []interface{}{"location", "oblast", 0}, out: "", err: NotFound},
	{args: []interface{}{0}, out: "", err: NotArray},
}

func TestFieldArrayStr(t *testing.T) {
	// run the tests
	for i, jt := range fieldArrayStrTests {
		out, err := parsedJsonData.FieldArrayStr(jt.args...)
		assert.Equal(t, jt.err, err, fmt.Sprintf("test %d, error", i))
		assert.Equal(t, jt.out, out, fmt.Sprintf("test %d, output", i))
	}
}
