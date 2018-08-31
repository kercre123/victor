package dev

import "net/http"

func Init() {
	initConnect()
}

func AddHandlers(mux *http.ServeMux) {
	mux.HandleFunc("/connect.html", connectHandler)
}
