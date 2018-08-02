// +build linux

package main

import (
	"anki/log"
	"encoding/base64"
	"strings"

	"golang.org/x/net/context"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/status"
)

const (
	Port                   = 443
	SocketPath             = "/dev/socket/"
	IsSwitchboardAvailable = true
)

func readAuthToken(ctx context.Context) (string, error) {
	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return "", status.Errorf(codes.Internal, "Failed to extract context metadata")
	}
	log.Printf("Received metadata: %+v", md)

	if len(md["Authorization"]) == 0 {
		return "", nil
	}
	authHeader := md["Authorization"][0]
	if !ok {
		return "", status.Errorf(codes.Unauthenticated, "User must provide an auth token")
	}
	if strings.HasPrefix(authHeader, "Basic ") {
		_, err := base64.StdEncoding.DecodeString(authHeader[6:])
		if err != nil {
			return "", status.Errorf(codes.Unauthenticated, "Failed to decode auth token (Base64)")
		}
		// todo
	} else if strings.HasPrefix(authHeader, "Bearer ") {
		return authHeader[7:], nil
	}
	return "", status.Errorf(codes.Unauthenticated, "Invalid auth type")
}

func verifyAuthToken(ctx context.Context) error {
	_, err := readAuthToken(ctx)
	return err
}
