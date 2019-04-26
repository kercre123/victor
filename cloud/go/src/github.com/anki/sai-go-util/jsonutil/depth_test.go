package jsonutil

import (
	"encoding/json"
	"strings"
	"testing"
)

type depthTestVector struct {
	ExpDepth int
	Json     string
}

func makeNestedJson(start, middle, end string, count int) string {
	return strings.Repeat(start, count) + middle + strings.Repeat(end, count)
}

func TestUnmarshaledMapDepth(t *testing.T) {
	testVectors := []depthTestVector{
		depthTestVector{
			ExpDepth: 1,
			Json:     `{}`,
		},
		depthTestVector{
			ExpDepth: 1,
			Json:     `{ "Field": "Value" }`,
		},
		depthTestVector{
			ExpDepth: 1,
			Json:     `{ "Field": null }`,
		},
		depthTestVector{
			ExpDepth: 2,
			Json:     `{ "Field": {} }`,
		},
		depthTestVector{
			ExpDepth: 2,
			Json:     `{ "Field": [] }`,
		},
		depthTestVector{
			ExpDepth: 3,
			Json:     `{ "Field": { "SubField": [ 1, 3] } }`,
		},
		depthTestVector{
			ExpDepth: 4,
			Json: `{
             	 "foo": "1",
             	 "one": {
             	  	"two": [["three"],"another three", {
             		 	"4v": 4
             		 }]
             	 }
             }`,
		},
	}
	// add some programmaticly-generated tests to testVectors
	for depth := 1; depth <= 20; depth++ {
		js := makeNestedJson(`{ "Field": `, `"Value"`, ` }`, depth)
		testVectors = append(testVectors, depthTestVector{ExpDepth: depth, Json: js})

		js = makeNestedJson(`{ "Field1": "Value1", "Field2": `, `"Value"`, ` }`, depth)
		testVectors = append(testVectors, depthTestVector{ExpDepth: depth, Json: js})

		js = makeNestedJson(`{ "Field": [`, `"Value"`, ` ] }`, depth)
		testVectors = append(testVectors, depthTestVector{ExpDepth: 2 * depth, Json: js})
	}

	// run the tests
	for _, vec := range testVectors {
		var jmap map[string]interface{}
		if err := json.Unmarshal([]byte(vec.Json), &jmap); err != nil {
			t.Fatal(err)
		}
		gotDepth := UnmarshaledMapDepth(jmap)
		if gotDepth != vec.ExpDepth {
			t.Errorf("Depth mismatch for JSON: %s\nExp=%d, got=%d\n", vec.Json, vec.ExpDepth, gotDepth)
		}
	}
}
