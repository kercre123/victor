package opentracing

import (
	lightstep "github.com/lightstep/lightstep-tracer-go"
	opentracing "github.com/opentracing/opentracing-go"
)

var (
	OpenTracer       opentracing.Tracer
	OpenTracerApiKey string
)

func Init() {
	// TODO: provision OpenTracerApiKey
	OpenTracer = lightstep.NewTracer(lightstep.Options{
		AccessToken: OpenTracerApiKey,
	})

	opentracing.SetGlobalTracer(OpenTracer)
}
