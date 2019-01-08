package opentracing

import (
	"anki/log"

	lightstep "github.com/lightstep/lightstep-tracer-go"
	opentracing "github.com/opentracing/opentracing-go"
)

var (
	OpenTracer       opentracing.Tracer
	OpenTracerApiKey string
)

func Init() {
	if OpenTracerApiKey != "" {
		log.Println("Initializing LightStep Open Tracing")

		OpenTracer = lightstep.NewTracer(lightstep.Options{
			AccessToken: OpenTracerApiKey,
		})

		opentracing.SetGlobalTracer(OpenTracer)
	} else {
		log.Println("Skipping LightStep Open Tracing initialization (no API key provided")
	}
}
