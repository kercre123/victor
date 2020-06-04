package vrscmd

import (
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/anki/sai-virtualrewards/client/virtualrewards"
)

func newClient(cfg *config.Config) *virtualrewards.VirtualRewardsClient {
	apicfg, err := apiutil.ApiClientCfg(cfg, config.VirtualRewards)
	if err != nil {
		cliutil.Fail(err.Error())
	}
	client, err := virtualrewards.NewClient("sai-go-cli", apicfg...)
	if err != nil {
		cliutil.Fail(err.Error())
	}
	return client
}
