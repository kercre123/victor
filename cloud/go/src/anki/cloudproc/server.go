package cloudproc

import (
	"net/http"
)

type handlerAdder func(*http.ServeMux) [][]string

var addHandlerFunc func(handlerAdder)

func addHandlers(f handlerAdder) {
	if addHandlerFunc != nil && f != nil {
		addHandlerFunc(f)
	}
}
