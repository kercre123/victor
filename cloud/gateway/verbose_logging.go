package main

import (
	"anki/log"
	"net/http"
)

type WrappedResponseWriter struct {
	http.ResponseWriter
	tag string
}

func (wrap *WrappedResponseWriter) WriteHeader(code int) {
	log.Printf("%s.WriteHeader(): %d", wrap.tag, code)
	wrap.ResponseWriter.WriteHeader(code)
}

func (wrap *WrappedResponseWriter) Write(b []byte) (int, error) {
	if wrap.tag == "json" && len(b) > 1 {
		log.Printf("%s.Write(): %s", wrap.tag, string(b))
	} else if wrap.tag == "grpc" && len(b) > 1 {
		log.Printf("%s.Write(): [% x]", wrap.tag, b)
	}
	return wrap.ResponseWriter.Write(b)
}

func (wrap *WrappedResponseWriter) Header() http.Header {
	h := wrap.ResponseWriter.Header()
	log.Printf("%s.Header(): %+v", wrap.tag, h)
	return h
}

func (wrap *WrappedResponseWriter) Flush() {
	wrap.ResponseWriter.(http.Flusher).Flush()
}

func (wrap *WrappedResponseWriter) CloseNotify() <-chan bool {
	return wrap.ResponseWriter.(http.CloseNotifier).CloseNotify()
}

func LogRequest(r *http.Request, tag string) {
	log.Printf("%s.Request: %+v", tag, r)
}
