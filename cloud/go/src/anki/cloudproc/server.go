package cloudproc

import (
	"net/http"
)

var addHandlerFunc func(func(*http.ServeMux))

func addHandlers(f func(*http.ServeMux)) {
	if addHandlerFunc != nil && f != nil {
		addHandlerFunc(f)
	}
}
