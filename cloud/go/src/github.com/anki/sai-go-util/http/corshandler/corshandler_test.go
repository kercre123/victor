// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package corshandler

import (
	"net/http"
	"net/http/httptest"
	"testing"
	"time"
)

var matchTests = []struct {
	pattern     string
	target      string
	shouldMatch bool
}{
	{"exact", "exact", true},
	{"exact", "exactish", false},
	{"exact", "notexact", false},
	{"*suffix", "suffix", true},
	{"*suffix", "asuffix", true},
	{"*suffix", "suffixed", false},
	{"*suffix", "fail", false},

	{"prefix*", "prefix", true},
	{"prefix*", "prefixed", true},
	{"prefix*", "aprefixed", false},
	{"prefix*", "fail", false},
	{"wallace*gromit", "wallacegromit", true},
	{"wallace*gromit", "wallace&gromit", true},
	{"wallace*gromit", "wallace loves gromit", true},
	{"wallace*gromit", "more cheese gromit", false},
	{"wallace*gromit", "wallace needs wendsleydale", false},
	{"wildcard-*-header", "wildcard-match-header", true},
}

func TestIsMatch(t *testing.T) {
	for _, test := range matchTests {
		p := newPattern(lowerCase, test.pattern)
		if result := p.isMatch(test.target); result != test.shouldMatch {
			t.Errorf("pattern=%q target=%q expected=%t actual=%t",
				test.pattern, test.target, test.shouldMatch, result)
		}
	}
}

func BenchmarkPatternMatchMid(b *testing.B) {
	p := newPattern(lowerCase, "wallace*grommit")
	for i := 0; i < b.N; i++ {
		p.isMatch("more cheese gromit")
	}
}

func BenchmarkPatternMatchPrefix(b *testing.B) {
	p := newPattern(lowerCase, "wallace*")
	for i := 0; i < b.N; i++ {
		p.isMatch("wallace&grommit")
	}
}

var domainTests = []struct {
	origin        string
	expectedSet   bool
	expectedMatch bool
}{
	{"http://sub1.example.com/foo", true, true},
	{"http://sub1.example.com:8000/foo", true, true},
	{"http://www.sub1.example.com/foo", false, false},
	{"http://sub2.example.com/foo", true, true},
	{"http://www.sub2.example.com/foo", false, false},
	{"http://sub3.example.com/foo", false, false},
	{"http://www.sub3.example.com/foo", true, true},
	{"", false, false}, // don't send an origin header and don't expect and accepts response
}

func TestCorsDomainMatch(t *testing.T) {
	h := New(&Config{
		AllowOriginDomains: []string{"sub1.example.com", "sub2.example.com", "*.sub3.example.com"},
		Handler:            http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {}),
	})
	for _, test := range domainTests {
		r, _ := http.NewRequest("POST", "", nil)
		r.Header.Set(originHeader, test.origin)
		w := httptest.NewRecorder()
		h.ServeHTTP(w, r)
		headers := w.Header()
		_, isSet := headers[allowOriginHeader]
		if !isSet && test.expectedSet {
			t.Errorf("allow-origin header was not set for origin=%q", test.origin)
			continue
		}
		if isSet && !test.expectedSet {
			t.Errorf("allow-origin header was is set for origin=%q", test.origin)
			continue
		}
		if test.expectedMatch && headers.Get(allowOriginHeader) != test.origin {
			t.Errorf("allow-origin header does not match header origin=%q allow-origin=%q",
				test.origin, headers.Get(allowOriginHeader))
		}
	}
}

func TestAllowedHeadersSet(t *testing.T) {
	h := New(&Config{
		AllowOriginDomains: []string{"sub1.example.com", "sub2.example.com"},
		AllowedHeaders:     []string{"header1", "header2"},
		Handler:            http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {}),
	})

	r, _ := http.NewRequest("POST", "", nil)
	r.Header.Set(originHeader, "http://sub1.example.com")
	w := httptest.NewRecorder()
	h.ServeHTTP(w, r)
	if w.Code != 200 {
		t.Errorf("Incorrect code expected=200 actual=%d", w.Code)
	}
	header := w.Header().Get(allowHeadersHeader)
	if header != "header1,header2" {
		t.Errorf("Incorrect header response %#v", w.Header())
	}
}

var preflightTests = []struct {
	origin     string
	reqHeaders string
	reqMethod  string
	expectOk   bool
}{
	{"http://incorrect.com", "good-header", "POST", false},
	{"http://example.com", "bad-header", "POST", false},
	{"http://example.com", "good-header, bad-header", "POST", false},
	{"http://example.com", "good-header", "POST", true},
	{"http://example.com", "Good-Header", "POST", true},
	{"http://example.com", "another-good-header", "POST", true},
	{"http://example.com", "good-header, another-good-header", "POST", true},
	{"http://example.com", "wildcard-match-header", "POST", true},
	{"http://example.com", "Wildcard-Match-Header", "POST", true},
	{"http://example.com", "wildCARD-whatever-HEADER", "POST", true},
	{"http://example.com", "another-GOOD-header", "POST", true},
	{"http://EXAMPLE.com", "another-GOOD-header", "POST", true},
	{"http://example.com", "", "", false},
	{"http://example.com", "", "POST", true},
	{"http://example.com", "", "GET", true},
	{"http://example.com", "", "FETCH", false},
	{"example.com", "", "POST", false}, // managled url
}

func TestPreflight(t *testing.T) {
	h := New(&Config{
		AllowOriginDomains: []string{"example.com"},
		AllowedHeaders:     []string{"good-header", "another-good-header", "wildcard-*-header"},
		AllowedMethods:     []string{"POST", "GET"},
	})
	for _, test := range preflightTests {
		handlerRan := false
		h.handler = http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { handlerRan = true })

		r, _ := http.NewRequest("OPTIONS", "", nil)
		r.Header.Set(originHeader, test.origin)
		r.Header.Set(requestMethodHeader, test.reqMethod)
		r.Header.Set(requestHeadersHeader, test.reqHeaders)
		w := httptest.NewRecorder()
		h.ServeHTTP(w, r)
		if handlerRan {
			// handler should never be called for preflight
			t.Errorf("Handler was called")
		}
		headers := w.Header()
		if !test.expectOk {
			if w.Code == http.StatusOK {
				t.Errorf("Unexpected OK for test=%v", test)
			}
			continue
		}
		if w.Code == http.StatusBadRequest {
			t.Errorf("Did not get good request response for test=%v", test)
		}
		if headers.Get(allowOriginHeader) != test.origin {
			t.Errorf("incorrect origin header expected=%q actual+%q, for test=%v",
				test.origin, headers.Get(allowOriginHeader), test)
		}
		if headers.Get(allowMethodsHeader) != "POST,GET" {
			t.Errorf("incorrect method header expected=%q actual+%q, for test=%v",
				"POST,GET", headers.Get(allowMethodsHeader), test)
		}
		expectedHeaders := test.reqHeaders
		if expectedHeaders == "" {
			// should send back the pattern list instead
			expectedHeaders = "good-header,another-good-header,wildcard-*-header"
		}
		if headers.Get(allowHeadersHeader) != expectedHeaders {
			t.Errorf("incorrect header header expected=%q actual=%q, for test=%v",
				expectedHeaders, headers.Get(allowHeadersHeader), test)
		}

	}
}

func TestNonPreflightOptions(t *testing.T) {
	handlerRan := false
	h := New(&Config{
		AllowOriginDomains: []string{"example.com"},
		AllowedHeaders:     []string{"good-header", "another-good-header"},
		AllowedMethods:     []string{"POST", "GET"},
		Handler:            http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { handlerRan = true }),
	})

	r, _ := http.NewRequest("OPTIONS", "", nil)
	w := httptest.NewRecorder()
	h.ServeHTTP(w, r)
	if !handlerRan {
		t.Errorf("Handler was not called")
	}
	if w.Header().Get(allowOriginHeader) != "" {
		t.Errorf("CORS headers were set unexpectedly")
	}
}

func TestCredentialsHeader(t *testing.T) {
	for _, setIt := range []bool{true, false} {
		h := New(&Config{
			AllowOriginDomains: []string{"example.com"},
			AllowedMethods:     []string{"POST"},
			AllowCredentials:   setIt,
			Handler:            http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {}),
		})

		// check pre-flight
		r, _ := http.NewRequest("OPTIONS", "", nil)
		r.Header.Set(originHeader, "http://example.com")
		r.Header.Set(requestMethodHeader, "POST")
		w := httptest.NewRecorder()
		h.ServeHTTP(w, r)
		if w.Code != http.StatusOK {
			t.Errorf("Didn't get OK response; body=%s", w.Body.String())
			continue
		}
		if setIt {
			if w.Header().Get(allowCredentialsHeader) != "true" {
				t.Errorf("preflight header not set headers=%#v", w.Header())
			}
		} else {
			if w.Header().Get(allowCredentialsHeader) != "" {
				t.Errorf("preflight header set headers=%#v", w.Header())
			}
		}

		// check request
		r, _ = http.NewRequest("POST", "", nil)
		r.Header.Set(originHeader, "http://example.com")
		w = httptest.NewRecorder()
		h.ServeHTTP(w, r)
		if w.Code != http.StatusOK {
			t.Errorf("Didn't get OK response; body=%s", w.Body.String())
			continue
		}
		if setIt {
			if w.Header().Get(allowCredentialsHeader) != "true" {
				t.Errorf("request header not set headers=%#v", w.Header())
			}
		} else {
			if w.Header().Get(allowCredentialsHeader) != "" {
				t.Errorf("preflight header set headers=%#v", w.Header())
			}
		}

	}
}

func TestMaxAge(t *testing.T) {
	for _, maxAge := range []time.Duration{0, 5 * time.Second} {
		h := New(&Config{
			AllowOriginDomains: []string{"example.com"},
			AllowedMethods:     []string{"POST"},
			MaxAge:             maxAge,
			Handler:            http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {}),
		})

		// check pre-flight
		r, _ := http.NewRequest("OPTIONS", "", nil)
		r.Header.Set(originHeader, "http://example.com")
		r.Header.Set(requestMethodHeader, "POST")
		w := httptest.NewRecorder()
		h.ServeHTTP(w, r)
		if w.Code != http.StatusOK {
			t.Errorf("Didn't get OK response; body=%s", w.Body.String())
			continue
		}

		if int(maxAge) > 0 {
			if w.Header().Get(allowMaxAgeHeader) != "5" {
				t.Errorf("maxage set header incorrect headers=%#v", w.Header())
			}
		} else {
			if w.Header().Get(allowMaxAgeHeader) != "" {
				t.Errorf("maxage unset header incorrect headers=%#v", w.Header())
			}
		}
	}
}
