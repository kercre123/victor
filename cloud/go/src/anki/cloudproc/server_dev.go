// +build !shipping

package cloudproc

import (
	"anki/cloudproc/dev"
	"net/http"
)

var serveMux *http.ServeMux

func init() {
	serveMux = http.NewServeMux()
	devServer = launchServer
	addHandlerFunc = func(f func(*http.ServeMux)) {
		f(serveMux)
	}
}

func launchServer() error {
	fs := http.FileServer(http.Dir("/anki/data/assets/cozmo_resources/webserver/cloud"))
	serveMux.Handle("/", fs)
	dev.Init()
	dev.AddHandlers(serveMux)
	return http.ListenAndServe(":8890", serveMux)
}
