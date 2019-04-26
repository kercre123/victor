package xfwdhandler

import (
	"net/http"
	"net/http/httptest"
	"testing"
)

var remoteHostTests = []struct {
	reqAddr          string
	xFwdAddr         string
	expectedEnabled  string
	expectedDisabled string
}{
	{"1.1.1.1:10000", "", "1.1.1.1:10000", "1.1.1.1:10000"},
	{"1.1.1.1", "", "1.1.1.1", "1.1.1.1"},
	{"1.1.1.1", "2.2.2.2,3.3.3.3", "3.3.3.3", "1.1.1.1"},
	{"1.1.1.1", "2.2.2.2, 4.4.4.4 ", "4.4.4.4", "1.1.1.1"},
	{"1.1.1.1:10000", "2.2.2.2:8000", "2.2.2.2:8000", "1.1.1.1:10000"},
	{"[::1]:10000", "[::2]:10000", "[::2]:10000", "[::1]:10000"},
}

func TestRemoteHost(t *testing.T) {
	var remoteAddr string
	h := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		remoteAddr = r.RemoteAddr
	})

	w := httptest.NewRecorder()

	for _, test := range remoteHostTests {
		req, _ := http.NewRequest("GET", "", nil)
		req.RemoteAddr = test.reqAddr

		Handler(h).ServeHTTP(w, req)
		if remoteAddr != test.expectedDisabled {
			t.Errorf("mismatch noFF reqAddr=%q xFwdAddr=%q expected=%q actual=%q",
				test.reqAddr, test.xFwdAddr, test.expectedDisabled, remoteAddr)
		}

		req.Header.Set("X-Forwarded-For", test.xFwdAddr)
		Handler(h).ServeHTTP(w, req)
		if remoteAddr != test.expectedEnabled {
			t.Errorf("mismatch withFF reqAddr=%q xFwdAddr=%q expected=%q actual=%q",
				test.reqAddr, test.xFwdAddr, test.expectedEnabled, remoteAddr)
		}
	}
}
