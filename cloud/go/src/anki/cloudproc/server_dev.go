// +build !shipping

package cloudproc

import (
	"anki/cloudproc/dev"
	"fmt"
	"html/template"
	"net/http"
)

var serveMux *http.ServeMux
var endpoints [][]string

func init() {
	serveMux = http.NewServeMux()
	devServer = launchServer
	addHandlerFunc = func(f handlerAdder) {
		added := f(serveMux)
		endpoints = append(endpoints, added...)
	}
}

func launchServer() error {
	serveMux.HandleFunc("/", indexHandler)
	dev.Init()
	dev.AddHandlers(serveMux)
	return http.ListenAndServe(":8890", serveMux)
}

func indexHandler(w http.ResponseWriter, r *http.Request) {
	t, err := template.ParseFiles("/anki/data/assets/cozmo_resources/webserver/cloud/index.html")
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		fmt.Fprint(w, "Error parsing template: ", err)
		return
	}

	if err := t.Execute(w, endpoints); err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		fmt.Fprint(w, "Error executing template: ", err)
		return
	}
}
