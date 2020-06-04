package ankival

import (
	"github.com/anki/sai-go-ankival/client/ankival"
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
)

func newClient(cfg *config.Config) *ankival.AnkivalClient {
	apicfg, err := apiutil.ApiClientCfg(cfg, config.Ankival)
	if err != nil {
		cliutil.Fail(err.Error())
	}
	client, err := ankival.NewAnkivalClient("sai-go-cli", apicfg...)
	if err != nil {
		cliutil.Fail(err.Error())
	}
	return client
}
