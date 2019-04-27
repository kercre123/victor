package interceptors

import (
	"github.com/anki/sai-go-util/log"
	"github.com/grpc-ecosystem/go-grpc-middleware/recovery"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
)

// RecoveryFactory returns server interceptors which convert panics
// into gRPC error return values with the `codes.Internal` return code
// set.
type RecoveryFactory struct{}

func (f *RecoveryFactory) UnaryServerInterceptor() grpc.UnaryServerInterceptor {
	return grpc_recovery.UnaryServerInterceptor(grpc_recovery.WithRecoveryHandler(logPanic))
}

func (f *RecoveryFactory) StreamServerInterceptor() grpc.StreamServerInterceptor {
	return grpc_recovery.StreamServerInterceptor(grpc_recovery.WithRecoveryHandler(logPanic))
}

func logPanic(p interface{}) error {
	alog.Panic{
		"status": "panic",
		"panic":  p,
	}.Log()
	return grpc.Errorf(codes.Internal, "%s", p)
}
