package simpleauth

import (
	"fmt"
	"net/http"
	"net/http/httptest"
	"os"
	"reflect"
	"testing"
)

var handlerTests = []struct {
	username string
	password string
	expected int
}{
	{"user1", "pw1", http.StatusOK},
	{"user1", "", http.StatusUnauthorized},
	{"user1", "xxx", http.StatusUnauthorized},
	{"user2", "pw1", http.StatusUnauthorized},
	{"user2", "pw2", http.StatusOK},
	{"user2", "", http.StatusUnauthorized},
	{"user2", "xxx", http.StatusUnauthorized},
	{"user3", "pw1", http.StatusUnauthorized},
	{"", "pw1", http.StatusUnauthorized},
	{"", "", http.StatusUnauthorized},
}

func TestHandlerResponse(t *testing.T) {
	testHandler := &Handler{
		Users: map[string]string{"user1": "pw1", "user2": "pw2"},
		Handler: http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			fmt.Fprintln(w, "OK")
		}),
	}

	for _, test := range handlerTests {
		r, _ := http.NewRequest("GET", "http://example.com/", nil)
		if test.username != "" || test.password != "" {
			r.SetBasicAuth(test.username, test.password)
		}
		w := httptest.NewRecorder()
		testHandler.ServeHTTP(w, r)
		if w.Code != test.expected {
			t.Errorf("username=%q password=%q expected=%d actual=%d",
				test.username, test.password, test.expected, w.Code)
		}
	}
}

func TestHandlerFromEnv(t *testing.T) {
	const envName = "simpleauth-test"
	os.Setenv(envName, "user1:pw1,user2:pw2")
	target := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprint(w, "OK")
	})

	w := httptest.NewRecorder()
	h := HandlerFromEnv(envName, target)

	h.Handler.ServeHTTP(w, nil)
	if buf := w.Body.String(); buf != "OK" {
		t.Error("Target handler was not set correctly")
	}

	expected := map[string]string{
		"user1": "pw1",
		"user2": "pw2",
	}
	if !reflect.DeepEqual(h.Users, expected) {
		t.Errorf("Handler had incorrect user map: %#v", h.Users)
	}
}
