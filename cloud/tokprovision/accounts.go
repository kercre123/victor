package main

import (
	"ankidev/accounts"
	"fmt"

	"github.com/anki/sai-go-cli/config"
)

func getToken() (string, error) {
	cfg, err := config.Load("", true, "dev", "default")
	if cfg != nil && err == nil {
		s, err := cfg.GetSession("default")
		if s != nil && err == nil && s.Token != "" && getBoolQuestion("You have a default SAI config "+
			"with a session token, possibly from using sai-go-cli before - should we use it?") {
			return s.Token, nil
		}
	}

	if getBoolQuestion("Do you think you already have a dev account to log in to?") {
		return tryLogin()
	}
	fmt.Println("Okay, let's create an account!")
	return tryCreate()
}

func tryLogin() (string, error) {
	user := getStringPrompt("Enter your username")
	pass := getPasswordPrompt("Enter your password")
	fmt.Println("Logging in...")
	s, _, err := accounts.DoLogin(user, pass)
	if err != nil {
		return "", err
	}
	if getBoolQuestion("Login successful! Do you want to save your session info to disk for future use " +
		"of this tool or other SAI (e.g. blobstore) tools? This may overwrite existing login info, but if " +
		"you had any, you would have been prompted to use it when you started this tool.") {
		s.Save()
	}
	return s.Token, nil
}

func tryCreate() (string, error) {
	email := getStringPrompt("Enter desired email address")
	fmt.Println("Checking if", email, "is available...")
	if ok, err := accounts.CheckUsername(email); err != nil {
		return "", err
	} else if !ok {
		return "", fmt.Errorf("Email %s already has an account", email)
	}
	password := getPasswordPrompt("Enter password")

	json, err := accounts.DoCreate(email, password)
	if err != nil {
		return "", err
	}
	token, err := json.FieldStr("session", "session_token")
	if err != nil {
		return "", err
	}
	return token, nil
}
