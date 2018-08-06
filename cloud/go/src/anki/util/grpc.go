package util

import (
	"context"
)

// TODO: make this configurable
const ankiDevKey = "xiepae8Ach2eequiphee4U"

type rpcMetadata map[string]string

func (r rpcMetadata) GetRequestMetadata(context.Context, ...string) (map[string]string, error) {
	return r, nil
}

func (r rpcMetadata) RequireTransportSecurity() bool {
	return true
}

func GrpcMetadata(jwtToken string) rpcMetadata {
	ret := rpcMetadata{
		"anki-app-key": ankiDevKey,
	}
	if jwtToken != "" {
		ret["anki-access-token"] = jwtToken
	}
	return ret
}
