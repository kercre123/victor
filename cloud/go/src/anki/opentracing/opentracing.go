package opentracing

import (
	"anki/log"
	"context"
	"errors"
	"fmt"
	"strconv"
	"strings"

	lightstep "github.com/lightstep/lightstep-tracer-go"
	opentracing "github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"
)

var (
	OpenTracer       opentracing.Tracer
	OpenTracerApiKey string
)

func Init(componentName string) {
	if OpenTracerApiKey != "" {
		log.Printf("Initializing LightStep Open Tracing with key: %q\n", OpenTracerApiKey)

		tags := make(opentracing.Tags)
		if componentName != "" {
			tags[lightstep.ComponentNameKey] = componentName
		}

		OpenTracer = lightstep.NewTracer(lightstep.Options{
			AccessToken: OpenTracerApiKey,
			Tags:        tags,
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
	clientSpan, _ := opentracing.StartSpanFromContext(ctx, operationName)

	ext.SpanKind.Set(clientSpan, "CLAD client")

	err := OpenTracer.Inject(clientSpan.Context(), opentracing.Binary, &spanContextString)
	if err != nil {
		log.Println("Error injecting span context:", err)
	}

	log.Printf("StartCladSpanFromContext: span = %q (operation = %q, ctx = %v)\n", spanContextString, operationName, ctx)

	return clientSpan, spanContextString
}

func createWireContext(spanContextString string) (opentracing.SpanContext, error) {
	if strings.ContainsAny(spanContextString, ",") {
		// base64 does not contains strings, so we assume CSV string
		parts := strings.Split(spanContextString, ",")
		if len(parts) != 2 {
			return nil, errors.New("invalid csv string")
		}

		traceID, err := strconv.ParseUint(parts[0], 10, 64)
		if err != nil {
			return nil, errors.New("can not parse traceID")
		}

		spanID, err := strconv.ParseUint(parts[1], 10, 64)
		if err != nil {
			return nil, errors.New("can not parse spanID")
		}

		spanContext := &lightstep.SpanContext{
			TraceID: traceID,
			SpanID:  spanID,
		}

		return spanContext, nil
	}

	return OpenTracer.Extract(opentracing.Binary, &spanContextString)
}

// ContextFromCladSpan de-serializes the SpanContext from the wire (i.e. a CLAD span context
// string property) and creates a new Span that is added to a new Context that is returned.
// See https://github.com/opentracing/opentracing-go#deserializing-from-the-wire
func ContextFromCladSpan(ctx context.Context, operationName string, spanContextString string) (opentracing.Span, context.Context) {
	log.Printf("ContextFromCladSpan: span = %q (operation = %q, ctx = %v)\n", spanContextString, operationName, ctx)

	var serverSpan opentracing.Span
	if spanContextString != "" {
		wireContext, err := createWireContext(spanContextString)
		if err != nil {
			log.Println("Error extracting/creating span context:", err)
		}
		fmt.Println("ContextFromCladSpan wireContext:", wireContext)

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
