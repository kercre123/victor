// +build !shipping

package cloudproc

import (
	"anki/cloudproc/dev"
	"anki/token"
	"net/http"
)

var serveMux *http.ServeMux

func init() {
	serveMux = http.NewServeMux()
	devServer = launchServer
	addHandlerFunc = func(f func(*http.ServeMux), s *token.Server) {
		f(serveMux)
		token.TokenServer = s
	}
}

func launchServer() error {
	fs := http.FileServer(http.Dir("/anki/data/assets/cozmo_resources/webserver/cloud"))
	serveMux.Handle("/", fs)
	dev.Init()
	dev.AddHandlers(serveMux)
	return http.ListenAndServe(":8890", serveMux)
}
