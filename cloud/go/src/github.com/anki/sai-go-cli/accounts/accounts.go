package accounts

import (
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func InitializeCommands(cmd *cli.Cmd, cfg **config.Config) {
	CheckUsernameCommand(cmd, cfg)
	CreateAccountCommand(cmd, cfg)
	DeactivateUsercommand(cmd, cfg)
	GetUserCommand(cmd, cfg)
	ListUsersCommand(cmd, cfg)
	ReactivateUsercommand(cmd, cfg)
	ResendVerifyCommand(cmd, cfg)
	ResetPasswordCommand(cmd, cfg)
	FindRelatedCommand(cmd, cfg)

	DeleteSessionCommand(cmd, cfg)
	LoginCommand(cmd, cfg)
	LogoutCommand(cmd, cfg)
	NewSessionCommand(cmd, cfg)
	ValidateSessionCommand(cmd, cfg)

	CheckStatusCommand(cmd, cfg)
}
