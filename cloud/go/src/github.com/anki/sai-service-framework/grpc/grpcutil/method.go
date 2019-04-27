package grpcutil

import (
	"context"
	"time"
)

type MethodCallDescriptor struct {
	StartedAt   time.Time
	FullMethod  string
	MetricName  string
	Package     string
	Service     string
	Method      string
	ServerInfo  interface{}
	IsStreaming bool
}

type methodCallContextKey int

const methodKey methodCallContextKey = iota

func WithMethodCallDescriptor(ctx context.Context, d *MethodCallDescriptor) context.Context {
	return context.WithValue(ctx, methodKey, d)
}

func GetMethodCallDescriptor(ctx context.Context) *MethodCallDescriptor {
	if d, ok := ctx.Value(methodKey).(*MethodCallDescriptor); ok {
		return d
	}
	return nil
}

func MustGetMethodCallDescriptor(ctx context.Context) *MethodCallDescriptor {
	if m := GetMethodCallDescriptor(ctx); m != nil {
		return m
	}
	panic("Must add MethodDecorator interceptor earlier in the chain")
}
