// +build !shipping

package cloudproc

import "net/http"

var serveMux *http.ServeMux

func init() {
	serveMux = http.NewServeMux()
	devServer = launchServer
	addHandlerFunc = func(f func(*http.ServeMux)) {
		f(serveMux)
	}
}

func launchServer() error {
	return http.ListenAndServe(":8890", serveMux)
}
