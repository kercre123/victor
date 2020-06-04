package jsonutil

import (
	"encoding/json"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"reflect"
	"strings"
	"testing"
)

func TestJsonErrorFromJson(t *testing.T) {
	text := `{"code": "acode", "message": "amessage", "status": "error", "meta1": 42, "meta2": "foobar"}`
	var result *JsonErrorResponse
	err := json.Unmarshal([]byte(text), &result)
	if err != nil {
		t.Fatal("Failed to unmarshal: ", err)
	}
	expected := &JsonErrorResponse{
		Code:    "acode",
		Message: "amessage",
		Metadata: map[string]interface{}{
			"meta1": float64(42),
			"meta2": "foobar",
		},
	}

	if !reflect.DeepEqual(expected, result) {
		t.Errorf("Didn't get expected result, got %#v", result)
	}
}

var etagTests = []struct {
	method            string
	inmHeader         string
	etagResponse      string
	expectNotModified bool
}{
	{"GET", "", "etag-set", false},
	{"GET", "non-match", "etag-set", false},
	{"GET", "etag-set", "etag-set", true},
	{"HEAD", "etag-set", "etag-set", true},
	{"POST", "etag-set", "etag-set", false},
	{"GET", "*", "etag-set", true},
	{"GET", "etag-set", "", false},
}

func TestJsonEtagResponse(t *testing.T) {
	data := "data"
	for _, test := range etagTests {
		recorder := httptest.NewRecorder()
		w := JsonWriterAdapter{ResponseWriter: recorder}
		r, _ := http.NewRequest(test.method, "/", nil)
		if test.inmHeader != "" {
			r.Header.Set("If-None-Match", test.inmHeader)
		}
		if err := w.JsonOkEtagData(r, test.etagResponse, data); err != nil {
			t.Errorf("method=%q inm=%q etag=%q error from JsonOkEtagData err=%v",
				test.method, test.inmHeader, test.etagResponse, err)
		}

		body := recorder.Body.String()
		if test.expectNotModified {
			if recorder.Code != http.StatusNotModified {
				t.Errorf("method=%q inm=%q etag=%q Did not get not modified code, got code=%d",
					test.method, test.inmHeader, test.etagResponse, recorder.Code)
			} else if body != "" {
				t.Errorf("method=%q inm=%q etag=%q Got unexpected body from not modified response body=%q",
					test.method, test.inmHeader, test.etagResponse, body)
			}

		} else {
			if recorder.Code != http.StatusOK {
				t.Errorf("method=%q inm=%q etag=%q Did not get not ok code, got code=%d",
					test.method, test.inmHeader, test.etagResponse, recorder.Code)
			}
			expectedBody := "\"" + data + "\"\n" // json encoded
			if body != expectedBody {
				t.Errorf("method=%q inm=%q etag=%q Got unexpected body from 200 response body=%q expected=%q",
					test.method, test.inmHeader, test.etagResponse, body, expectedBody)
			}
		}
		if etag := recorder.Header().Get("Etag"); etag != test.etagResponse {
			t.Errorf("method=%q inm=%q etag=%q etag header was not set correctly, got %q",
				test.method, test.inmHeader, test.etagResponse, etag)
		}

	}
}

var readLimitTests = []struct {
	name    string
	ct      string
	data    string
	limit   int
	expCode string
}{
	{"ok", "text/json", `{"key": "value"}`, 1000, ""},
	{"too-big", "text/json", `{"key": "value"}`, 10, "bad_input"},
	{"bad-ct", "text/plain", `{"key": "value"}`, 1000, "bad_content_type"},
}

func TestReadRequestJsonLimit(t *testing.T) {
	for _, test := range readLimitTests {
		r := &http.Request{
			Header: http.Header{"Content-Type": []string{test.ct}},
			Body:   ioutil.NopCloser(strings.NewReader(test.data)),
		}
		data, err := ReadRequestJsonLimit(r, test.limit)
		if err != nil {
			if code := err.JsonError().Code; code != test.expCode {
				t.Errorf("name=%s incorrect code expected=%q actual=%q", test.name, test.expCode, code)
			}
		} else {
			if test.expCode != "" {
				t.Errorf("name=%s error code %s, received none", test.name, test.expCode)
			} else if string(data) != test.data {
				t.Errorf("name=%s incorrect data %#v", test.name, data)
			}
		}
	}
}

var parseLimitTests = []struct {
	name    string
	ct      string
	data    string
	limit   int
	expCode string
}{
	{"ok", "text/json", `{"key": "value"}`, 1000, ""},
	{"too-big", "text/json", `{"key": "value"}`, 10, "bad_input"},
	{"bad-ct", "text/plain", `{"key": "value"}`, 1000, "bad_content_type"},
	{"bad-json", "text/json", `"key": "value"`, 1000, "bad_input"},
}

func TestParseRequestJsonLimit(t *testing.T) {
	for _, test := range parseLimitTests {
		var data struct {
			Key string `json:"key"`
		}
		r := &http.Request{
			Header: http.Header{"Content-Type": []string{test.ct}},
			Body:   ioutil.NopCloser(strings.NewReader(test.data)),
		}
		err := ParseRequestJsonLimit(r, test.limit, &data)
		if err != nil {
			if code := err.JsonError().Code; code != test.expCode {
				t.Errorf("name=%s incorrect code expected=%q actual=%q", test.name, test.expCode, code)
			}
		} else {
			if test.expCode != "" {
				t.Errorf("name=%s error code %s, received none", test.name, test.expCode)
			} else if data.Key != "value" {
				t.Errorf("name=%s incorrect data %#v", test.name, data)
			}
		}
	}
}
