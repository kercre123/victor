package dev

import (
	"anki/util/cvars/web"
	"net/http"
	"net/http/pprof"
)

func Init() {
	initConnect()
}

func AddHandlers(mux *http.ServeMux) {
	mux.HandleFunc("/debug/pprof/", pprof.Index)
	mux.HandleFunc("/debug/pprof/cmdline", pprof.Cmdline)
	mux.HandleFunc("/debug/pprof/profile", pprof.Profile)
	mux.HandleFunc("/debug/pprof/symbol", pprof.Symbol)
	mux.HandleFunc("/debug/pprof/trace", pprof.Trace)
	mux.HandleFunc("/connect.html", connectHandler)
	mux.HandleFunc("/cvars", web.ListCVars)
	mux.HandleFunc("/cvars/req", web.CVarHandler)
}
