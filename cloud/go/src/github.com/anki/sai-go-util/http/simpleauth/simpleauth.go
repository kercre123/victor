// Package simpleauth provides basic HTTP authentication support.

package simpleauth

import (
	"net/http"
	"os"
	"strings"
)

// Handler provides HTTP basic authentication checks for an http.Handler.
//
// It holds a plaintext map of acceptable usernames and passwords and
// returns an unauthorized response to any requests not providing a matching
// username/password.
type Handler struct {
	Users   map[string]string // Mapping of usernames to clear-text passwords
	Handler http.Handler
}

func (h *Handler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	username, password, ok := r.BasicAuth()
	if !ok {
		http.Error(w, "Permission Denied", http.StatusUnauthorized)
		return
	}

	if h.Users[username] != password {
		http.Error(w, "Permission Denied", http.StatusUnauthorized)
		return
	}

	h.Handler.ServeHTTP(w, r)
}

// Generates a user Handler by parsing an environment variable to configure usernames and passwords.
// The environment variable must be a comma separate list of username:password pairs
func HandlerFromEnv(envName string, wrapHandler http.Handler) *Handler {
	h := &Handler{Users: make(map[string]string), Handler: wrapHandler}

	pairs := strings.Split(os.Getenv(envName), ",")
	for _, entry := range pairs {
		userpass := strings.SplitN(entry, ":", 2)
		if len(userpass) == 2 {
			h.Users[userpass[0]] = userpass[1]
		}
	}
	return h
}
