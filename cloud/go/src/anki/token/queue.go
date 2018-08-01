package token

import (
	"anki/log"
	"anki/robot"
	"anki/token/jwt"
	"clad/cloud"
	"fmt"
	"time"

	"google.golang.org/grpc/credentials"
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

func queueInit(done <-chan struct{}) error {
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
	go queueRoutine(done)
	return nil
}

func handleQueueRequest(req *request) error {
	var err error
	var resp *cloud.TokenResponse
	switch req.m.Tag() {
	case cloud.TokenRequestTag_Auth:
		resp, err = handleAuthRequest(req.m.GetAuth().SessionToken)
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
			c, err := getConnection(getTokenMetadata(existing.String()))
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

func handleAuthRequest(session string) (*cloud.TokenResponse, error) {
	errorResp := func(code cloud.TokenError) *cloud.TokenResponse {
		return cloud.NewTokenResponseWithAuth(&cloud.AuthResponse{Error: code})
	}

	c, err := getConnection(getAuthMetadata(session))
	if err != nil {
		return errorResp(cloud.TokenError_Connection), err
	}
	defer c.Close()

	bundle, err := c.associatePrimary(session, robotESN)
	if err != nil {
		return errorResp(cloud.TokenError_Connection), err
	}
	_, err = jwt.ParseToken(bundle.Token)
	if err != nil {
		return errorResp(cloud.TokenError_InvalidToken), err
	}
	return cloud.NewTokenResponseWithAuth(&cloud.AuthResponse{
		AppToken: bundle.ClientToken,
		JwtToken: bundle.Token}), nil
}

func queueRoutine(done <-chan struct{}) {
	for {
		var req request
		select {
		case <-done:
			return
		case req = <-queue:
			break
		}
		if err := handleQueueRequest(&req); err != nil {
			log.Println("Token queue error:", err)
		}
	}
}
