package token

import "net/http"

var devHandlers func(*http.ServeMux)

func GetDevHandlers(s *http.ServeMux) [][]string {
	if devHandlers != nil {
		devHandlers(s)
	}
	return nil
}

var SetDevServer = func(s *Server) {}
