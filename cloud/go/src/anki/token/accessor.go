package token

import (
	"anki/token/jwt"
	"anki/util"
	"clad/cloud"
	"fmt"

	"google.golang.org/grpc/credentials"
)

type Accessor interface {
	Credentials() (credentials.PerRPCCredentials, error)
	UserID() string
}

type accessor struct{}

func (accessor) Credentials() (credentials.PerRPCCredentials, error) {
	req := cloud.NewTokenRequestWithJwt(&cloud.JwtRequest{})
	resp, err := HandleRequest(req)
	if err != nil {
		return nil, err
	}
	jwt := resp.GetJwt()
	if jwt.Error != cloud.TokenError_NoError {
		return nil, fmt.Errorf("jwt error code %d", jwt.Error)
	}
	return tokenMetadata(resp.GetJwt().JwtToken), nil
}

func (accessor) UserID() string {
	token := jwt.GetToken()
	if token == nil {
		return ""
	}
	return token.UserID()
}

func GetAccessor() Accessor {
	return accessor{}
}

func tokenMetadata(jwtToken string) util.MapCredentials {
	ret := util.AppkeyMetadata()
	ret["anki-access-token"] = jwtToken
	return ret
}
