package chipper

import (
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

type connOpts struct {
	deviceID  string
	sessionID string
	grpcOpts  []grpc.DialOption
	insecure  bool
	creds     credentials.PerRPCCredentials
	firmware  string
	bootID    string
}

// ConnOpt represents an option that can be used to change connection behavior
type ConnOpt func(o *connOpts)

// WithDeviceID specifies the device ID to provide the server
func WithDeviceID(value string) ConnOpt {
	return func(o *connOpts) {
		o.deviceID = value
	}
}

// WithInsecure specifies whether the connection with chipper should be insecure
func WithInsecure(value bool) ConnOpt {
	return func(o *connOpts) {
		o.insecure = value
	}
}

// WithSessionID specifies the session ID to provide the server
func WithSessionID(value string) ConnOpt {
	return func(o *connOpts) {
		o.sessionID = value
	}
}

// WithFirmwareVersion specifies the firmware version to provide the server
func WithFirmwareVersion(value string) ConnOpt {
	return func(o *connOpts) {
		o.firmware = value
	}
}

// WithGrpcOptions adds the given GRPC DialOption values to those
// we pass to GRPC when creating a connection
func WithGrpcOptions(value ...grpc.DialOption) ConnOpt {
	return func(o *connOpts) {
		o.grpcOpts = append(o.grpcOpts, value...)
	}
}

// WithCredentials adds the given per RPC credentials to GRPC requests
func WithCredentials(value credentials.PerRPCCredentials) ConnOpt {
	return func(o *connOpts) {
		o.creds = value
	}
}

// WithBootID will include the given boot ID with requests
func WithBootID(value string) ConnOpt {
	return func(o *connOpts) {
		o.bootID = value
	}
}
