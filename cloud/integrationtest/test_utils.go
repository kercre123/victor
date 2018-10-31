package main

import (
	"ankidev/accounts"
	"fmt"

	"github.com/anki/sai-go-cli/config"
	"github.com/anki/sai-go-util/http/apiclient"
	"github.com/anki/sai-token-service/client/token"
	"github.com/anki/sai-token-service/model"
)

func getCredentials(tokenClient *tokenClient) (*model.Token, error) {
	jwtResponse, err := tokenClient.Jwt()
	if err != nil {
		return nil, err
	}

	return token.NewValidator().TokenFromString(jwtResponse.JwtToken)
}

func createTestAccount(envName, userName, password string) (apiclient.Json, error) {
	if _, err := config.Load("", true, envName, "default"); err != nil {
		return nil, err
	}

	if ok, err := accounts.CheckUsername(envName, userName); err != nil {
		return nil, err
	} else if !ok {
		fmt.Printf("Email %s already has an account\n", userName)
		return nil, nil
	}

	return accounts.DoCreate(envName, userName, password)
}
