package opentracing

import (
	"anki/log"
	"context"

	lightstep "github.com/lightstep/lightstep-tracer-go"
	opentracing "github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"
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
		log.Println("Skipping LightStep Open Tracing initialization (no API key provided)")
	}
}

// StartCladSpanFromContext creates a new child Span using the SpanContext found in the ctx Context as
// a parent. It subsequently serializes its SpanContext in a string that is sent between processes using
// a string property in a CLAD message.
// See https://github.com/opentracing/opentracing-go#serializing-to-the-wire
func StartCladSpanFromContext(ctx context.Context, operationName string) (opentracing.Span, string) {
	var spanContextString string
	span, _ := opentracing.StartSpanFromContext(ctx, operationName)

	ext.SpanKind.Set(serverSpan, "CLAD client")

	err := OpenTracer.Inject(span.Context(), opentracing.Binary, &spanContextString)
	if err != nil {
		log.Println("Error injecting span context:", err)
	}

	log.Printf("StartCladSpanFromContext: span = %q (operation = %q, ctx = %v)\n", spanContextString, operationName, ctx)

	return span, spanContextString
}

// ContextFromCladSpan de-serializes the SpanContext from the wire (i.e. a CLAD span context
// string property) and creates a new Span that is added to a new Context that is returned.
// See https://github.com/opentracing/opentracing-go#deserializing-from-the-wire
func ContextFromCladSpan(ctx context.Context, operationName string, spanContextString string) (opentracing.Span, context.Context) {
	log.Printf("ContextFromCladSpan: span = %q (operation = %q, ctx = %v)\n", spanContextString, operationName, ctx)

	var serverSpan opentracing.Span
	if spanContextString != "" {
		wireContext, err := OpenTracer.Extract(opentracing.Binary, &spanContextString)
		if err != nil {
			log.Println("Error extracting span context:", err)
		}

		// TODO: this new span may again be created in the client interceptor, to be looked into.
		serverSpan = opentracing.StartSpan(operationName, ext.RPCServerOption(wireContext))
	} else {
		log.Printf("Skipping span extraction: %q\n", spanContextString)

		// TODO: this new span may again be created in the client interceptor, to be looked into.
		serverSpan = opentracing.StartSpan(operationName)
	}

	ext.SpanKind.Set(serverSpan, "CLAD server")

	newCtxt := ctx
	if newCtxt == nil {
		newCtxt = context.Background()
	}

	return serverSpan, opentracing.ContextWithSpan(newCtxt, serverSpan)
}
