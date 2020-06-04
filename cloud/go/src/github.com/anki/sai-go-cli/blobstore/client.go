package blobstore

import (
	"github.com/anki/sai-blobstore/client/blobstore"
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
)

func newClient(cfg *config.Config) *blobstore.Client {
	apicfg, err := apiutil.ApiClientCfg(cfg, config.Blobstore)
	if err != nil {
		cliutil.Fail(err.Error())
	}
	client, err := blobstore.New("sai-go-cli", apicfg...)
	if err != nil {
		cliutil.Fail(err.Error())
	}
	return client
}
