package token

import (
	"anki/token/identity"
	"anki/util"
	"clad/cloud"
	"fmt"

	ac "github.com/aws/aws-sdk-go/aws/credentials"
	gc "google.golang.org/grpc/credentials"
)

type Accessor interface {
	Credentials() (gc.PerRPCCredentials, error)
	GetStsCredentials() (*ac.Credentials, error)
	IdentityProvider() identity.Provider
	UserID() string
}

type accessor struct {
	identityProvider identity.Provider
}

func (accessor) Credentials() (gc.PerRPCCredentials, error) {
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

func (a accessor) GetStsCredentials() (*ac.Credentials, error) {
	return getStsCredentials(a)
}

func (a accessor) UserID() string {
	token := a.identityProvider.GetToken()
	if token == nil {
		return ""
	}
	return token.UserID()
}

func (a accessor) IdentityProvider() identity.Provider {
	return a.identityProvider
}

func GetAccessor(identityProvider identity.Provider) Accessor {
	return accessor{identityProvider}
}

func tokenMetadata(jwtToken string) util.MapCredentials {
	ret := util.AppkeyMetadata()
	ret["anki-access-token"] = jwtToken
	return ret
}
