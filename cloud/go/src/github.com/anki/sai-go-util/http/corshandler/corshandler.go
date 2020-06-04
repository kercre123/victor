// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// corshandler implements an HTTP handler for
// Cross-origin resource sharing queries.
// It implements the specification located at http://www.w3.org/TR/cors/
package corshandler

import (
	"fmt"
	"net"
	"net/http"
	"net/textproto"
	"net/url"
	"strconv"
	"strings"
	"time"
)

const (
	originHeader           = "Origin"
	requestMethodHeader    = "Access-Control-Request-Method"
	requestHeadersHeader   = "Access-Control-Request-Headers"
	allowOriginHeader      = "Access-Control-Allow-Origin"
	allowMethodsHeader     = "Access-Control-Allow-Methods"
	allowHeadersHeader     = "Access-Control-Allow-Headers"
	allowMaxAgeHeader      = "Access-Control-Max-Age"
	allowCredentialsHeader = "Access-Control-Allow-Credentials"
)

func csIsEqual(a, b string) bool {
	return a == b
}

func nonCsIsEqual(a, b string) bool {
	return strings.ToLower(a) == strings.ToLower(b)
}

// pattern type
type ptype int

const (
	ptExact = iota
	ptNone
	ptWildcard
	ptPrefix
	ptSuffix
	ptMid
)

// pattern case handling
type pcase int

const (
	lowerCase = iota
	upperCase
	canonicalMime
)

// a "compiled" pattern
type pattern struct {
	ptype  ptype
	prefix string // also used for equals matches
	suffix string
}

func (p pattern) isMatch(target string) bool {
	switch p.ptype {
	case ptNone:
		return false
	case ptWildcard:
		return true
	case ptExact:
		return target == p.prefix
	case ptPrefix:
		return len(target) >= len(p.prefix) && target[0:len(p.prefix)] == p.prefix
	case ptSuffix:
		return len(target) >= len(p.suffix) && target[len(target)-len(p.suffix):] == p.suffix
	case ptMid:
		return len(target) >= len(p.prefix)+len(p.suffix) && target[0:len(p.prefix)] == p.prefix && target[len(target)-len(p.suffix):] == p.suffix
	}
	panic("Unknown pattern type")
}

// pre-process a pattern
func newPattern(pcase pcase, pstr string) pattern {
	switch pcase {
	case lowerCase:
		pstr = strings.ToLower(pstr)
	case upperCase:
		pstr = strings.ToUpper(pstr)
	case canonicalMime:
		pstr = textproto.CanonicalMIMEHeaderKey(pstr)
	default:
		panic("Unknown pattern case")
	}

	if len(pstr) == 1 {
		if pstr == "*" {
			return pattern{ptype: ptWildcard}
		}
		return pattern{ptype: ptExact, prefix: pstr}
	}

	switch {
	case pstr == "":
		// doesn't make much sense that this would ever be true
		return pattern{ptype: ptNone}
	case pstr[0] == '*':
		return pattern{ptype: ptSuffix, suffix: pstr[1:]}
	case pstr[len(pstr)-1] == '*':
		return pattern{ptype: ptPrefix, prefix: pstr[0 : len(pstr)-1]}
	}

	if pos := strings.IndexByte(pstr, '*'); pos > -1 {
		pprefix, psuffix := pstr[0:pos], pstr[pos+1:]
		return pattern{ptype: ptMid, prefix: pprefix, suffix: psuffix}
	}

	return pattern{ptype: ptExact, prefix: pstr}
}

// pre-process a list of patterns
func newPatterns(pcase pcase, pstrs []string) (patterns []pattern) {
	if len(pstrs) == 0 {
		return nil
	}
	patterns = make([]pattern, len(pstrs))
	for i, pstr := range pstrs {
		patterns[i] = newPattern(pcase, pstr)
	}
	return patterns
}

// anyMatch returns true if any of the supplied patterns is a match for a target string.
func anyMatch(patterns []pattern, target string) bool {
	for _, p := range patterns {
		if p.isMatch(target) {
			return true
		}
	}
	return false
}

// Config defines parameters to supply to New to generate
// a new CorsHandler.
//
// AllowOriginDomains, AllowMethods and AllowHeaders define string patterns
// to match against requests.  They may contain an exact string, or a wildcard
// utilizing at most one star symbol.
//
// Supplying nil for AllowOriginDomains, AllowedMethods or AllowedHeaders
// causes them to match any request for their type, as if it had been set
// to []string{"*"}
type Config struct {
	AllowOriginDomains []string
	AllowedMethods     []string
	AllowedHeaders     []string
	AllowCredentials   bool
	MaxAge             time.Duration
	Handler            http.Handler
}

// CorsHandler wraps an HTTP handler and sets the Access-Control-Allow-Origin
// response header to match the request's origin header, if the origin
// is a domain or sub-domain match against a list of allowed origins.
//
// The handler also handles preflight requests sent using the OPTIONS method.
// such requests are not relaeyd to the wrapped handler.
// OPTIONS requests that do not set an Origin header are passed through, however.
type CorsHandler struct {
	allowOriginDomains []pattern
	allowedMethods     []pattern
	allowedHeaders     []pattern
	allowedMethodsList string
	allowedHeadersList string
	allowCredentials   bool
	maxAge             time.Duration
	handler            http.Handler
}

// New accepts a Config and returns an initialized CorsHandler
func New(config *Config) *CorsHandler {
	var (
		allowedOriginDomains = config.AllowOriginDomains
		allowedMethods       = config.AllowedMethods
		allowedHeaders       = config.AllowedHeaders
	)
	if len(allowedOriginDomains) == 0 {
		allowedOriginDomains = []string{"*"}
	}
	if len(allowedMethods) == 0 {
		allowedMethods = []string{"*"}
	}
	if len(allowedHeaders) == 0 {
		allowedHeaders = []string{"*"}
	}
	return &CorsHandler{
		allowOriginDomains: newPatterns(lowerCase, allowedOriginDomains),
		allowedMethods:     newPatterns(upperCase, allowedMethods),
		allowedHeaders:     newPatterns(lowerCase, allowedHeaders),
		allowedMethodsList: strings.Join(allowedMethods, ","),
		allowedHeadersList: strings.Join(allowedHeaders, ","),
		allowCredentials:   config.AllowCredentials,
		maxAge:             config.MaxAge,
		handler:            config.Handler,
	}
}

func (h *CorsHandler) originAllowed(origin string) bool {
	u, err := url.ParseRequestURI(origin)
	if err != nil {
		return false
	}
	host := strings.ToLower(u.Host)
	if h, _, err := net.SplitHostPort(host); err == nil {
		host = h
	}
	if anyMatch(h.allowOriginDomains, host) {
		return true
	}
	return false
}

func (h *CorsHandler) handlePreflight(w http.ResponseWriter, r *http.Request) {
	origin := r.Header.Get(originHeader)
	if origin == "" {
		// not a preflight request after all; perhpas some other handler will deal with  it
		h.handler.ServeHTTP(w, r)
		return
	}

	if !h.originAllowed(origin) {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Invalid Origin"))
		return
	}

	method := r.Header.Get(requestMethodHeader)
	if method == "" {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Method not specified"))
		return
	}

	if !anyMatch(h.allowedMethods, method) {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Method not allowed"))
		return
	}

	headerNames := r.Header.Get(requestHeadersHeader)
	if headerNames != "" {
		for _, name := range strings.Split(headerNames, ",") {
			name = strings.ToLower(strings.TrimSpace(name))
			if !anyMatch(h.allowedHeaders, name) {
				w.WriteHeader(http.StatusBadRequest)
				w.Write([]byte(fmt.Sprintf("Header %q not allowed", name)))
				return
			}
		}
	}

	if h.maxAge > time.Duration(0) {
		w.Header().Set(allowMaxAgeHeader, strconv.FormatInt(int64(h.maxAge.Seconds()), 10))
	}

	w.Header().Set(allowOriginHeader, origin)
	w.Header().Set(allowMethodsHeader, h.allowedMethodsList)
	if headerNames == "" {
		w.Header().Set(allowHeadersHeader, h.allowedHeadersList)
	} else {
		w.Header().Set(allowHeadersHeader, headerNames)
	}
	if h.allowCredentials {
		w.Header().Set(allowCredentialsHeader, "true")
	}
	w.WriteHeader(200)
}

func (h *CorsHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	w.Header().Add("Vary", "Origin")

	if r.Method == "OPTIONS" {
		h.handlePreflight(w, r)
		return
	}

	origin := r.Header.Get(originHeader)
	if origin == "" {
		h.handler.ServeHTTP(w, r)
		return
	}

	if !h.originAllowed(origin) {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Invalid Origin"))
		return
	}

	if h.allowCredentials {
		w.Header().Set(allowCredentialsHeader, "true")
	}

	w.Header().Set(allowOriginHeader, origin)

	if len(h.allowedHeaders) > 0 {
		w.Header().Set(allowHeadersHeader, h.allowedHeadersList)
	}
	h.handler.ServeHTTP(w, r)
}
