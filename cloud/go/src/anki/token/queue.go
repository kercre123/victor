package token

import (
	"anki/log"
	"anki/robot"
	"anki/token/jwt"
	"anki/util"
	"clad/cloud"
	"context"
	"fmt"
	"time"

	pb "github.com/anki/sai-token-service/proto/tokenpb"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/status"
)

type request struct {
	m  *cloud.TokenRequest
	ch chan *response
}

type response struct {
	resp *cloud.TokenResponse
	err  error
}

var defaultESN func() string

var queue = make(chan request)
var url = "token-dev.api.anki.com:443"
var robotESN string

func queueInit(ctx context.Context) error {
	esn, err := robot.ReadESN()
	if err != nil {
		if defaultESN == nil {
			if err := robot.WriteFaceErrorCode(852); err != nil {
				log.Println("Couldn't print face error:", err)
			}
			return fmt.Errorf("error reading ESN: %s", err.Error())
		}
		esn = defaultESN()
	}
	robotESN = esn
	go queueRoutine(ctx)
	return nil
}

func handleQueueRequest(req *request) error {
	var err error
	var resp *cloud.TokenResponse
	switch req.m.Tag() {
	case cloud.TokenRequestTag_Auth:
		resp, err = handleAuthRequest(req.m.GetAuth().SessionToken)
	case cloud.TokenRequestTag_Secondary:
		resp, err = handleSecondaryAuthRequest(req.m.GetSecondary())
	case cloud.TokenRequestTag_Jwt:
		resp, err = handleJwtRequest()
	}
	req.ch <- &response{resp, err}
	return err
}

func getConnection(creds credentials.PerRPCCredentials) (*conn, error) {
	c, err := newConn(url, creds)
	if err != nil {
		return nil, err
	}
	return c, nil
}

// this function has two representations of errors - any error object returned
// by a request should be returned for logging by processing code, but we need to
// generate a CLAD response for token requests no matter what, and those responses
// should indicate the stage of the request where an error occurred
func handleJwtRequest() (*cloud.TokenResponse, error) {
	existing := jwt.GetToken()
	errorResp := func(code cloud.TokenError) *cloud.TokenResponse {
		return cloud.NewTokenResponseWithJwt(&cloud.JwtResponse{Error: code})
	}
	tokenResp := func(token string) *cloud.TokenResponse {
		return cloud.NewTokenResponseWithJwt(&cloud.JwtResponse{JwtToken: token})
	}
	if existing != nil {
		if time.Now().After(existing.RefreshTime()) {
			c, err := getConnection(tokenMetadata(existing.String()))
			if err != nil {
				return errorResp(cloud.TokenError_Connection), err
			}
			defer c.Close()
			bundle, err := c.refreshToken(existing.String())
			if err != nil {
				return errorResp(cloud.TokenError_Connection), err
			}
			tok, err := jwt.ParseToken(bundle.Token)
			if err != nil {
				return errorResp(cloud.TokenError_InvalidToken), err
			}
			return tokenResp(tok.String()), nil
		}
		return tokenResp(existing.String()), nil
	}
	// no token: this is an error for whoever we're sending the CLAD response to,
	// because they asked for a token and we can't give them one, but it's not
	// technically an error for our own queue-processing functionality
	return errorResp(cloud.TokenError_NullToken), nil
}

func authErrorResp(code cloud.TokenError) *cloud.TokenResponse {
	return cloud.NewTokenResponseWithAuth(&cloud.AuthResponse{Error: code})
}

func handleAuthRequest(session string) (*cloud.TokenResponse, error) {
	metadata := util.AppkeyMetadata()
	metadata["anki-user-session"] = session
	requester := func(c *conn) (*pb.TokenBundle, error) {
		return c.associatePrimary(session, robotESN)
	}
	return authRequester(metadata, requester, true)
}

func handleSecondaryAuthRequest(req *cloud.SecondaryAuthRequest) (*cloud.TokenResponse, error) {
	existing := jwt.GetToken()
	if existing == nil {
		return authErrorResp(cloud.TokenError_NullToken), nil
	}

	metadata := tokenMetadata(existing.String())
	requester := func(c *conn) (*pb.TokenBundle, error) {
		return c.associateSecondary(existing.String(), req.SessionToken, req.ClientName, req.AppId)
	}
	return authRequester(metadata, requester, false)
}

func authRequester(creds credentials.PerRPCCredentials,
	requester func(c *conn) (*pb.TokenBundle, error),
	parseJwt bool) (*cloud.TokenResponse, error) {

	c, err := getConnection(creds)
	if err != nil {
		return authErrorResp(cloud.TokenError_Connection), err
	}
	defer c.Close()

	bundle, err := requester(c)
	if err != nil {
		if status.Code(err) == codes.InvalidArgument {
			return authErrorResp(cloud.TokenError_WrongAccount), err
		}
		return authErrorResp(cloud.TokenError_Connection), err
	}
	if parseJwt {
		_, err = jwt.ParseToken(bundle.Token)
		if err != nil {
			return authErrorResp(cloud.TokenError_InvalidToken), err
		}
	}
	return cloud.NewTokenResponseWithAuth(&cloud.AuthResponse{
		AppToken: bundle.ClientToken,
		JwtToken: bundle.Token}), nil
}

func queueRoutine(ctx context.Context) {
	for {
		var req request
		select {
		case <-ctx.Done():
			return
		case req = <-queue:
			break
		}
		if err := handleQueueRequest(&req); err != nil {
			log.Println("Token queue error:", err)
		}
	}
}
