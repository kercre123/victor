package audit

import (
	"github.com/anki/sai-accounts-audit/audit"
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
)

func newClient(cfg *config.Config) audit.Client {
	apicfg, err := apiutil.ApiClientCfg(cfg, config.Audit)
	if err != nil {
		cliutil.Fail(err.Error())
	}
	client, err := audit.NewClient(apicfg...)
	if err != nil {
		cliutil.Fail(err.Error())
	}
	return client
}
