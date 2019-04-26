package blobstore

import (
	"fmt"
	"net/http"
	"strings"

	"github.com/anki/sai-blobstore/client/blobstore"
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func SearchCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("search", "Search the blobstore", func(cmd *cli.Cmd) {
		cmd.Spec = "[--namespace] --key [--meta] [--op=<eq>] --value [--value2] [--limit=<10>]"

		namespace := cmd.StringOpt("n namespace", "cozmo", "Namespace to upload blob into")
		key := cmd.StringOpt("k key", "", "Metadata key to search on")
		op := cmd.StringOpt("o op", "eq", "Comparison to perform.  One of eq, lt, lte, gt, gte, prefix, between")
		value1 := cmd.StringOpt("v value", "", "Value to match")
		value2 := cmd.StringOpt("value2", "", "Second value for between operator")
		max := cmd.IntOpt("l limit", 10, "Maximum number of results to return")
		meta := cmd.StringsOpt("meta", nil, "Comma separated list of additional metadata keys to display")

		cmd.Action = func() {
			c := newClient(*cfg)

			if *max < 1 || *max > 20 {
				cliutil.Fail("--limit must be between 1 and 20")
			}

			s := blobstore.SearchParams{
				Key:        *key,
				Value:      *value1,
				Value2:     *value2,
				SearchMode: *op,
				MaxResults: *max,
			}

			resp, err := c.Search(*namespace, s)
			if len(*meta) == 0 || err != nil || resp.StatusCode != http.StatusOK {
				apiutil.DefaultHandler(resp, err)
				return
			}

			data, err := resp.Json()
			if err != nil {
				cliutil.Fail("Failed to decode JSON: %v", err)
			}

			results, ok := data["results"].([]interface{})
			if !ok {
				cliutil.Fail("No results")
			}

			mm := make(map[string]bool)
			for _, k := range *meta {
				mm[strings.ToLower(k)] = true
			}

			for _, result := range results {
				kvs, ok := result.(map[string]interface{})
				if !ok {
					cliutil.Fail("Unexpected entry in results")
				}
				id, ok := kvs["id"].(string)
				if !ok {
					cliutil.Fail("Entry without ID!")
				}
				// fetch full metadata
				md, err := c.FetchMeta(*namespace, id)
				if err != nil {
					cliutil.Fail(err.Error())
				}

				fmt.Println("ID:", kvs["id"])
				for k, v := range md {
					if k == "id" || (!mm["all"] && !mm[strings.ToLower(k)]) {
						continue
					}
					fmt.Printf("%s: %v\n", k, v)
				}
				fmt.Println("")
			}
		}
	})
}
