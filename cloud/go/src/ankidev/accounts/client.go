package accounts

import (
	"github.com/anki/sai-go-accounts/client/accounts"
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
)

func newClient() (*accounts.AccountsClient, *config.Config, error) {
	cfg, err := config.Load("", false, "dev", "default")
	if err != nil {
		return nil, nil, err
	}
	apicfg, err := apiutil.ApiClientCfg(cfg, config.Accounts)
	if err != nil {
		return nil, nil, err
	}
	c, err := accounts.NewAccountsClient("sai-go-cli", apicfg...)
	if err != nil {
		return nil, nil, err
	}
	return c, cfg, nil
}
