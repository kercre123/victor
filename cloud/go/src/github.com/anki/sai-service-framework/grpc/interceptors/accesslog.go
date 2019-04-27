package interceptors

import (
	"context"
	"fmt"
	"io"
	"strings"
	"time"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"github.com/anki/sai-service-framework/svc"
	"github.com/jawher/mow.cli"
	"github.com/stoewer/go-strcase"
	"google.golang.org/grpc"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/peer"
)

// AccessLogFactory returns client and server interceptors which
// automaticaly log all gRPC calls to Anki's log library, with all the
// typical fields you would find in e.g. an HTTP access log, including
// the service & method called, start time, elapsed duration, user
// agent, client ID address, and request ID.
type AccessLogFactory struct {
	dumpMetadata *bool
}

func NewAccessLogFactory() *AccessLogFactory {
	return &AccessLogFactory{}
}

func (f *AccessLogFactory) CommandSpec() string {
	return "[--grpc-dump-metadata]"
}

func (f *AccessLogFactory) CommandInitialize(cmd *cli.Cmd) {
	if f.dumpMetadata == nil {
		f.dumpMetadata = svc.BoolOpt(cmd, "grpc-dump-metadata", true,
			"If enabled, dump all outbound and incoming gRPC metadata to the access log")
	}
}

// logInboundMetadata logs all key/value pairs from any incoming gRPC
// metadata available in the context. The log statement is logged with
// LogC(ctx), which will also add the request ID, allowing the debug
// log statement to be joined with the regular access_log containing
// the rest of the gRPC invocation data.
func logInboundMetadata(ctx context.Context) {
	log := alog.Info{
		"action": "access_log_debug",
		"system": "grpc",
		"kind":   "server",
	}
	if md, ok := metadata.FromIncomingContext(ctx); ok {
		addMetadataToLog(log, md)
	}
	log.LogC(ctx)
}

// logOutboundMetadata logs all key/value pairs from any outgoing gRPC
// metadata available in the context. The log statement is logged with
// LogC(ctx), which will also add the request ID, allowing the debug
// log statement to be joined with the regular access_log containing
// the rest of the gRPC invocation data.
func logOutboundMetadata(ctx context.Context) {
	log := alog.Info{
		"action": "access_log_debug",
		"system": "grpc",
		"kind":   "client",
	}
	if md, ok := metadata.FromOutgoingContext(ctx); ok {
		addMetadataToLog(log, md)
	}
	log.LogC(ctx)
}

// addMetadataToLog populates a log object with the supplied gRPC
// metadata. Because metadata values can be multi-valent, each value
// is formated with the log key "meta.<fieldname>.<n>".
func addMetadataToLog(log alog.Info, md metadata.MD) {
	for k, vals := range md {
		for i, v := range vals {
			if k == grpcutil.AppKeyMetadataKey {
				if len(v) >= 3 {
					v = grpcutil.SanitizeAppKey(v)
				}
			}
			log[fmt.Sprintf("meta_%s_%d", strcase.SnakeCase(k), i)] = v
		}
	}
}

func getServerLogEntry(ctx context.Context, d *grpcutil.MethodCallDescriptor) alog.Info {
	log := alog.Info{
		"action":         "access_log",
		"system":         "grpc",
		"kind":           "server",
		"grpc_package":   d.Package,
		"grpc_service":   d.Service,
		"grpc_method":    d.Method,
		"grpc_streaming": d.IsStreaming,
		"start_time":     d.StartedAt.Format(time.RFC3339),
		"trace_id":       grpcutil.GetTraceId(ctx),
		"user_agent":     getUserAgent(ctx),
		"clientip":       getClientIP(ctx),
	}
	if peer := grpcutil.GetPeerCertificate(ctx); peer != nil {
		log["client_cert_name"] = peer.Subject.CommonName
	}
	return log
}

func getClientLogEntry(ctx context.Context, d *grpcutil.MethodCallDescriptor) alog.Info {
	return alog.Info{
		"action":         "access_log",
		"system":         "grpc",
		"kind":           "client",
		"grpc_package":   d.Package,
		"grpc_service":   d.Service,
		"grpc_method":    d.Method,
		"grpc_streaming": d.IsStreaming,
		"start_time":     d.StartedAt.Format(time.RFC3339),
		"trace_id":       grpcutil.GetTraceId(ctx),
	}
}

func getClientIP(ctx context.Context) string {
	if p, ok := peer.FromContext(ctx); ok {
		addr := p.Addr.String()
		return strings.Split(addr, ":")[0]
	}
	return ""
}

func getUserAgent(ctx context.Context) string {
	if md, ok := metadata.FromIncomingContext(ctx); ok {
		if userAgent, ok := md["user-agent"]; ok {
			return userAgent[0]
		}
	}
	return ""
}

func updateLogEntry(logentry *alog.Info, err error, d *grpcutil.MethodCallDescriptor) *alog.Info {
	duration := time.Now().Sub(d.StartedAt)
	(*logentry)["duration_ms"] = float32(duration.Nanoseconds()/1000) / 1000
	(*logentry)["grpc_code"] = grpc.Code(err).String()
	if err != nil {
		(*logentry)["error"] = err
	}
	return logentry
}

// ----------------------------------------------------------------------
// Interceptors
// ----------------------------------------------------------------------

func (f *AccessLogFactory) UnaryServerInterceptor() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		descriptor := grpcutil.MustGetMethodCallDescriptor(ctx)
		logentry := getServerLogEntry(ctx, descriptor)

		ret, err := handler(ctx, req)

		if f.dumpMetadata != nil && *f.dumpMetadata {
			logInboundMetadata(ctx)
		}
		updateLogEntry(&logentry, err, descriptor).LogC(ctx)
		return ret, err
	}
}

func (f *AccessLogFactory) StreamServerInterceptor() grpc.StreamServerInterceptor {
	return func(srv interface{}, stream grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		ctx := stream.Context()
		descriptor := grpcutil.MustGetMethodCallDescriptor(ctx)

		logentry := getServerLogEntry(ctx, descriptor)
		wrapper := &logWrappedServerStream{ServerStream: stream, descriptor: descriptor}

		err := handler(srv, wrapper)

		if f.dumpMetadata != nil && *f.dumpMetadata {
			logInboundMetadata(ctx)
		}
		logentry["send_count"] = wrapper.sendCount
		logentry["recv_count"] = wrapper.recvCount
		updateLogEntry(&logentry, err, descriptor).LogC(ctx)
		return err
	}
}

func (f *AccessLogFactory) UnaryClientInterceptor() grpc.UnaryClientInterceptor {
	return func(ctx context.Context, method string, req, reply interface{}, cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
		descriptor := grpcutil.MustGetMethodCallDescriptor(ctx)
		logentry := getClientLogEntry(ctx, descriptor)

		err := invoker(ctx, method, req, reply, cc, opts...)

		if f.dumpMetadata != nil && *f.dumpMetadata {
			logOutboundMetadata(ctx)
		}
		updateLogEntry(&logentry, err, descriptor).LogC(ctx)
		return err
	}
}

func (f *AccessLogFactory) StreamClientInterceptor() grpc.StreamClientInterceptor {
	return func(ctx context.Context, desc *grpc.StreamDesc, cc *grpc.ClientConn, method string, streamer grpc.Streamer, opts ...grpc.CallOption) (grpc.ClientStream, error) {
		descriptor := grpcutil.MustGetMethodCallDescriptor(ctx)

		if f.dumpMetadata != nil && *f.dumpMetadata {
			logOutboundMetadata(ctx)
		}
		s, err := streamer(ctx, desc, cc, method, opts...)
		return &logWrappedClientStream{
			ClientStream: s,
			descriptor:   descriptor,
			logentry:     getClientLogEntry(ctx, descriptor),
		}, err
	}
}

type logWrappedServerStream struct {
	grpc.ServerStream
	descriptor *grpcutil.MethodCallDescriptor
	sendCount  int
	recvCount  int
}

func (s *logWrappedServerStream) SendMsg(m interface{}) error {
	s.sendCount++
	return s.ServerStream.SendMsg(m)
}

func (s *logWrappedServerStream) RecvMsg(m interface{}) error {
	err := s.ServerStream.RecvMsg(m)
	if err != io.EOF {
		s.recvCount++
	}
	return err
}

type logWrappedClientStream struct {
	grpc.ClientStream
	descriptor *grpcutil.MethodCallDescriptor
	logentry   alog.Info
	sendCount  int
	recvCount  int
}

func (s *logWrappedClientStream) SendMsg(m interface{}) error {
	s.sendCount++
	return s.ClientStream.SendMsg(m)
}

func (s *logWrappedClientStream) RecvMsg(m interface{}) error {
	err := s.ClientStream.RecvMsg(m)
	if err != io.EOF {
		s.recvCount++
	}
	return err
}

func (s *logWrappedClientStream) CloseSend() error {
	err := s.ClientStream.CloseSend()

	s.logentry["send_count"] = s.sendCount
	s.logentry["recv_count"] = s.recvCount
	updateLogEntry(&s.logentry, err, s.descriptor).LogC(s.Context())

	return err
}
