// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// xfwdhandler provides an a wrapper for an HTTP handler
// that replaces the request's RemoteAddr field with that provided
// in an X-Forwarded-For header, if any.
package xfwdhandler

import (
	"net/http"
	"strings"
)

const (
	xForwardedFor = "X-Forwarded-For"
)

// Handler wraps an existing handler and replaces request.RemoteAddr
// with the address set in the X-Forwarded-For header, if any.
func Handler(handler http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if addr := r.Header.Get(xForwardedFor); addr != "" {
			if i := strings.LastIndex(addr, ","); i > -1 {
				// get the last proxy in the chain, if more than one
				// the last one is the *only* reliable address, and even
				// then its only reliable if you know your proxy will always
				// set the header.
				addr = trim(addr[i+1:])
			}
			r.RemoteAddr = addr
		}
		handler.ServeHTTP(w, r)
	})
}

func trim(s string) string {
	for len(s) > 0 && s[0] == ' ' {
		s = s[1:]
	}
	for len(s) > 0 && s[len(s)-1] == ' ' {
		s = s[:len(s)-1]
	}
	return s
}
