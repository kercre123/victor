package token

import (
	"anki/log"
	"anki/robot"
	"anki/token/jwt"
	"clad/cloud"
	"fmt"
	"time"
)

type request struct {
	m  *cloud.TokenRequest
	ch chan *response
}

type response struct {
	resp *cloud.TokenResponse
	err  error
}

var queue = make(chan request)
var url = "token-dev.api.anki.com:443"
var robotESN string

func queueInit(done <-chan struct{}) error {
	esn, err := robot.ReadESN()
	if err != nil {
		if err := robot.WriteFaceErrorCode(852); err != nil {
			log.Println("Couldn't print face error:", err)
		}
		return fmt.Errorf("error reading ESN: %s", err.Error())
	}
	robotESN = esn
	go queueRoutine(done)
	return nil
}

func handleQueueRequest(req *request, c *conn) (*conn, error) {
	var err error
	var resp *cloud.TokenResponse
	switch req.m.Tag() {
	case cloud.TokenRequestTag_Auth:
		resp, c, err = handleAuthRequest(c, req.m.GetAuth().SessionToken)
	case cloud.TokenRequestTag_Jwt:
		resp, c, err = handleJwtRequest(c)
	}
	req.ch <- &response{resp, err}
	return c, err
}

func getConnection(c *conn) (*conn, error) {
	if c != nil {
		return c, nil
	}
	c, err := newConn(url)
	if err != nil {
		return nil, err
	}
	return c, nil
}

// this function has two representations of errors - any error object returned
// by a request should be returned for logging by processing code, but we need to
// generate a CLAD response for token requests no matter what, and those responses
// should indicate the stage of the request where an error occurred
func handleJwtRequest(c *conn) (*cloud.TokenResponse, *conn, error) {
	existing := jwt.GetToken()
	errorResp := func(code cloud.TokenError) *cloud.TokenResponse {
		return cloud.NewTokenResponseWithJwt(&cloud.JwtResponse{Error: code})
	}
	tokenResp := func(token string) *cloud.TokenResponse {
		return cloud.NewTokenResponseWithJwt(&cloud.JwtResponse{JwtToken: token})
	}
	if existing != nil {
		if time.Now().After(existing.RefreshTime()) {
			c, err := getConnection(c)
			if err != nil {
				return errorResp(cloud.TokenError_Connection), nil, err
			}
			bundle, err := c.refreshToken(existing.String())
			if err != nil {
				return errorResp(cloud.TokenError_Connection), nil, err
			}
			tok, err := jwt.ParseToken(bundle.Token)
			if err != nil {
				return errorResp(cloud.TokenError_InvalidToken), nil, err
			}
			return tokenResp(tok.String()), c, nil
		}
		return tokenResp(existing.String()), c, nil
	}
	// no token: this is an error for whoever we're sending the CLAD response to,
	// because they asked for a token and we can't give them one, but it's not
	// technically an error for our own queue-processing functionality
	return errorResp(cloud.TokenError_NullToken), c, nil
}

func handleAuthRequest(c *conn, session string) (*cloud.TokenResponse, *conn, error) {
	c, err := getConnection(c)
	errorResp := func(code cloud.TokenError) *cloud.TokenResponse {
		return cloud.NewTokenResponseWithAuth(&cloud.AuthResponse{Error: code})
	}
	if err != nil {
		return errorResp(cloud.TokenError_Connection), nil, err
	}
	bundle, err := c.associatePrimary(session, robotESN)
	if err != nil {
		return errorResp(cloud.TokenError_Connection), nil, err
	}
	_, err = jwt.ParseToken(bundle.Token)
	if err != nil {
		return errorResp(cloud.TokenError_InvalidToken), nil, err
	}
	return cloud.NewTokenResponseWithAuth(&cloud.AuthResponse{
		AppToken: bundle.ClientToken,
		JwtToken: bundle.Token}), c, nil
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
		c, err := handleQueueRequest(&req, nil)
		// re-use the existing connection we just created
		// as long as queue still has stuff in it
	reuse:
		for c != nil && err == nil {
			select {
			case req = <-queue:
				c, err = handleQueueRequest(&req, c)
			default:
				if err = c.Close(); err != nil {
					log.Println("Error closing token connection:", err)
				}
				break reuse
			}
		}
	}
}
