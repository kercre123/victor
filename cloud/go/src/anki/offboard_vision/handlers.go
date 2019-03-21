package offboard_vision

import "net/http"

var devHandlers func(*http.ServeMux) [][]string

func GetDevHandlers(s *http.ServeMux) [][]string {
  if devHandlers != nil {
    return devHandlers(s)
  }
  return nil
}
