package main

import (
	"fmt"

	"github.com/anki/sai-go-accounts/models/user"
	"github.com/anki/sai-go-util/cli"
	"github.com/anki/sai-go-util/envconfig"
)

var hashPasswordCmd = &Command{
	Name:       "hash-password",
	RequiresDb: false,
	Exec:       hashPassword,
	Flags:      hashPasswordFlags,
}

var (
	password string
)

func hashPasswordFlags(cmd *Command) {
	envconfig.String(&password, "", "password", "Password to hash")
}

func hashPassword(cmd *Command, args []string) {
	pw, err := user.NewHashedPassword(user.DefaultHasher, []byte(password))
	if err != nil {
		fmt.Println("Failed to hash password", err)
		cli.Exit(2)
	}

	fmt.Println("Hashed to", pw)
}
