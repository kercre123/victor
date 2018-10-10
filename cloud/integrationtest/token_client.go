package main

import (
	"bytes"
	"clad/cloud"
	"fmt"
)

type tokenClient struct {
	cladClient
}

func (c *tokenClient) requestResponse(request *cloud.TokenRequest) (*cloud.TokenResponse, error) {
	var requestBuf bytes.Buffer
	if err := request.Pack(&requestBuf); err != nil {
		return nil, err
	}

	if _, err := c.conn.Write(requestBuf.Bytes()); err != nil {
		return nil, err
	}

	responseBuf := c.conn.ReadBlock()
	var responseMsg cloud.TokenResponse
	if err := responseMsg.Unpack(bytes.NewBuffer(responseBuf)); err != nil {
		return nil, err
	}

	return &responseMsg, nil
}

func (c *tokenClient) Auth(sessionToken string) (*cloud.AuthResponse, error) {
	responseMessage, err := c.requestResponse(cloud.NewTokenRequestWithAuth(&cloud.AuthRequest{sessionToken}))
	if err != nil {
		return nil, err
	}

	switch responseMessage.Tag() {
	case cloud.TokenResponseTag_Auth:
		response := responseMessage.GetAuth()
		return response, nil
	}
	return nil, fmt.Errorf("Major error: received unknown tag %d", responseMessage.Tag())
}

func (c *tokenClient) Reassociate() error {
	// TODO: not yet implemented
	return nil
}

func (c *tokenClient) SecondaryAuth() error {
	// TODO: not yet implemented
	return nil
}

func (c *tokenClient) Jwt() (*cloud.JwtResponse, error) {
	responseMessage, err := c.requestResponse(cloud.NewTokenRequestWithJwt(&cloud.JwtRequest{}))
	if err != nil {
		return nil, err
	}

	switch responseMessage.Tag() {
	case cloud.TokenResponseTag_Jwt:
		response := responseMessage.GetJwt()
		return response, nil
	}
	return nil, fmt.Errorf("Major error: received unknown tag %d", responseMessage.Tag())
}
