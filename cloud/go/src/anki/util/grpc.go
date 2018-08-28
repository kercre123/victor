package util

import (
	"anki/config"
	"context"
)

type MapCredentials map[string]string

func (r MapCredentials) GetRequestMetadata(context.Context, ...string) (map[string]string, error) {
	return r, nil
}

func (r MapCredentials) RequireTransportSecurity() bool {
	return true
}

func AppkeyMetadata() MapCredentials {
	ret := MapCredentials{
		"anki-app-key": config.Env.AppKey,
	}
	return ret
}
