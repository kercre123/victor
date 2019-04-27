package interceptors

import (
	"context"
	"sync"
	"time"

	"github.com/aalpern/go-metrics"
	"github.com/anki/sai-go-util/metricsutil"
	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
)

var (
	allCodes = []codes.Code{
		codes.OK, codes.Canceled, codes.Unknown, codes.InvalidArgument,
		codes.DeadlineExceeded, codes.NotFound, codes.AlreadyExists,
		codes.PermissionDenied, codes.Unauthenticated, codes.ResourceExhausted,
		codes.FailedPrecondition, codes.Aborted, codes.OutOfRange,
		codes.Unimplemented, codes.Internal, codes.Unavailable, codes.DataLoss,
	}
	defaultServerRegistry = metricsutil.NewRegistry("grpc.server")
	defaultClientRegistry = metricsutil.NewRegistry("grpc.client")
)

// MetricsFactory returns interceptors which automatically record
// metrics on gRPC calls, including call counts and latency per
// method, as well as counts per response code. Metrics names are
// under the grpc.server and grpc.client paths.
type MetricsFactory struct {
	serverRegistry metrics.Registry
	clientRegistry metrics.Registry
	codesCounters  map[codes.Code]metrics.Counter
	lock           sync.RWMutex
}

const (
	callResponsePrefix   = "call.response"
	streamResponsePrefix = "stream.response"
)

func NewMetricsFactory() *MetricsFactory {
	f := &MetricsFactory{
		serverRegistry: defaultServerRegistry,
		clientRegistry: defaultClientRegistry,
		codesCounters:  make(map[codes.Code]metrics.Counter),
	}
	return f
}

// These counters should all be pre-populated, but just in case gRPC
// adds more return codes, we won't miss them.
func (f *MetricsFactory) getCodeCounter(r metrics.Registry, prefix string, c codes.Code) metrics.Counter {
	f.lock.RLock()
	if counter, ok := f.codesCounters[c]; ok {
		f.lock.RUnlock()
		return counter
	} else {
		f.lock.RUnlock()
		counter = metricsutil.GetCounter(prefix+"."+c.String(), r)

		f.lock.Lock()
		f.codesCounters[c] = counter
		f.lock.Unlock()
		return counter
	}
}

// ----------------------------------------------------------------------
// Interceptors
// ----------------------------------------------------------------------

func (f *MetricsFactory) UnaryServerInterceptor() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		descriptor := grpcutil.MustGetMethodCallDescriptor(ctx)
		all := metricsutil.GetTimer("call.latency.all", f.serverRegistry)
		forThisMethod := metricsutil.GetTimer("call.latency.method."+descriptor.MetricName, f.serverRegistry)

		start := time.Now()
		defer all.UpdateSince(start)
		defer forThisMethod.UpdateSince(start)

		ret, err := handler(ctx, req)
		code := grpc.Code(err)
		f.getCodeCounter(f.serverRegistry, callResponsePrefix, code).Inc(1)
		return ret, err
	}
}

func (f *MetricsFactory) StreamServerInterceptor() grpc.StreamServerInterceptor {
	return func(srv interface{}, stream grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		descriptor := grpcutil.MustGetMethodCallDescriptor(stream.Context())
		start := time.Now()
		forThisMethod := metricsutil.GetTimer("stream.latency.method."+descriptor.MetricName, f.serverRegistry)
		defer forThisMethod.UpdateSince(start)

		err := handler(srv, &metricsWrappedServerStream{
			ServerStream: stream,
			f:            f,
			descriptor:   descriptor,
		})
		code := grpc.Code(err)
		f.getCodeCounter(f.serverRegistry, streamResponsePrefix, code).Inc(1)
		return err
	}
}

func (f *MetricsFactory) UnaryClientInterceptor() grpc.UnaryClientInterceptor {
	return func(ctx context.Context, method string, req, reply interface{}, cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
		descriptor := grpcutil.MustGetMethodCallDescriptor(ctx)
		all := metricsutil.GetTimer("call.latency.all", f.clientRegistry)
		forThisMethod := metricsutil.GetTimer("call.latency.method."+descriptor.MetricName, f.clientRegistry)

		start := time.Now()
		defer all.UpdateSince(start)
		defer forThisMethod.UpdateSince(start)

		err := invoker(ctx, method, req, reply, cc, opts...)

		code := grpc.Code(err)
		f.getCodeCounter(f.clientRegistry, callResponsePrefix, code).Inc(1)

		return err
	}
}

func (f *MetricsFactory) StreamClientInterceptor() grpc.StreamClientInterceptor {
	return func(ctx context.Context, desc *grpc.StreamDesc, cc *grpc.ClientConn, method string, streamer grpc.Streamer, opts ...grpc.CallOption) (grpc.ClientStream, error) {
		descriptor := grpcutil.MustGetMethodCallDescriptor(ctx)
		strm, err := streamer(ctx, desc, cc, method, opts...)
		code := grpc.Code(err)
		f.getCodeCounter(f.clientRegistry, streamResponsePrefix, code).Inc(1)
		return &metricsWrappedClientStream{
			ClientStream: strm,
			f:            f,
			descriptor:   descriptor,
		}, err
	}
}

type metricsWrappedServerStream struct {
	grpc.ServerStream
	f          *MetricsFactory
	descriptor *grpcutil.MethodCallDescriptor
}

func (s *metricsWrappedServerStream) SendMsg(m interface{}) error {
	all := metricsutil.GetTimer("stream.latency.all", s.f.serverRegistry)
	forThisMethod := metricsutil.GetTimer("stream.latency.method."+s.descriptor.MetricName+".send", s.f.serverRegistry)

	start := time.Now()
	defer all.UpdateSince(start)
	defer forThisMethod.UpdateSince(start)

	return s.ServerStream.SendMsg(m)
}

func (s *metricsWrappedServerStream) RecvMsg(m interface{}) error {
	all := metricsutil.GetTimer("stream.latency.all", s.f.serverRegistry)
	forThisMethod := metricsutil.GetTimer("stream.latency.method."+s.descriptor.MetricName+".recv", s.f.serverRegistry)

	start := time.Now()
	defer all.UpdateSince(start)
	defer forThisMethod.UpdateSince(start)

	return s.ServerStream.RecvMsg(m)
}

type metricsWrappedClientStream struct {
	grpc.ClientStream
	f          *MetricsFactory
	descriptor *grpcutil.MethodCallDescriptor
}

func (s *metricsWrappedClientStream) SendMsg(m interface{}) error {
	all := metricsutil.GetTimer("stream.latency.all", s.f.clientRegistry)
	forThisMethod := metricsutil.GetTimer("stream.latency.method."+s.descriptor.MetricName+".send", s.f.clientRegistry)

	start := time.Now()
	defer all.UpdateSince(start)
	defer forThisMethod.UpdateSince(start)

	return s.ClientStream.SendMsg(m)
}

func (s *metricsWrappedClientStream) RecvMsg(m interface{}) error {
	all := metricsutil.GetTimer("stream.latency.all", s.f.clientRegistry)
	forThisMethod := metricsutil.GetTimer("stream.latency.method."+s.descriptor.MetricName+".recv", s.f.clientRegistry)

	start := time.Now()
	defer all.UpdateSince(start)
	defer forThisMethod.UpdateSince(start)

	return s.ClientStream.RecvMsg(m)
}
