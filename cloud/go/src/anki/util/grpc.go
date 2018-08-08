package util

import (
	"context"
)

// TODO: make this configurable
const ankiDevKey = "xiepae8Ach2eequiphee4U"

type MapCredentials map[string]string

func (r MapCredentials) GetRequestMetadata(context.Context, ...string) (map[string]string, error) {
	return r, nil
}

func (r MapCredentials) RequireTransportSecurity() bool {
	return true
}

func AppkeyMetadata() MapCredentials {
	ret := MapCredentials{
		"anki-app-key": ankiDevKey,
	}
	return ret
}
