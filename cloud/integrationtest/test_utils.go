package main

import (
	"ankidev/accounts"
	"fmt"

	"github.com/anki/sai-go-cli/config"
	"github.com/anki/sai-go-util/http/apiclient"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-token-service/model"
	jwt "github.com/dgrijalva/jwt-go"
	"github.com/go-redis/redis"
)

func init() {
	alog.ToStdout()
}

func logIfNoError(err error, userName, action, format string, a ...interface{}) {
	if err != nil {
		alog.Error{
			"action":         action,
			"test_user_name": userName,
			"status":         "error",
			"error":          err,
		}.Log()
	} else {
		alog.Info{
			"action":         action,
			"test_user_name": userName,
			"status":         "ok",
			"message":        fmt.Sprintf(format, a...),
		}.Log()
	}
}

// Copied from "anki/token/jwt" (but not unexported)
func parseToken(token string) (*model.Token, error) {
	t, _, err := new(jwt.Parser).ParseUnverified(token, jwt.MapClaims{})
	if err != nil {
		return nil, err
	}
	tok, err := model.FromJwtToken(t)
	if err != nil {
		return nil, err
	}
	return tok, nil
}

func getCredentials(tokenClient *tokenClient) (*model.Token, error) {
	jwtResponse, err := tokenClient.Jwt()
	if err != nil {
		return nil, err
	}

	return parseToken(jwtResponse.JwtToken)
}

func getUniqueTestID(address string) (int64, error) {
	client := redis.NewClient(&redis.Options{Addr: address})
	return client.Incr("test_id").Result()
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
