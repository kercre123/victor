package accounts

import (
	"encoding/json"
	"fmt"
	"net/http"
	"net/url"
	"strconv"

	"github.com/agrison/go-tablib"
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func ListUsersCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("list-users", "Query user accounts", func(cmd *cli.Cmd) {
		// cmd.Spec = "[-u] [-i] [-f] [-e] [-m] [-l] [-o] [-s] [--sort-desc] [--order=<username>] [-t=<json>]"
		cmd.Spec = "[-l] [-o] [-s] [--sort-desc] [--order=<username>] [-t=<json>]"

		// username   := cmd.StringOpt("u username", "", "Filter by username")
		// userId     := cmd.StringOpt("i user-id", "", "Filter by user id")
		// familyName := cmd.StringOpt("f family-name", "", "Filter by family name")
		// email      := cmd.StringOpt("e email", "", "Filter by email address")
		// mode       := cmd.StringOpt("m mode", "", "Use exact match (default) or prefix when filtering on username, user id, family name, or email.")
		limit := cmd.IntOpt("l limit", 0, "Limit number of results")
		offset := cmd.IntOpt("o offset", 0, "Start result set from offset")
		sortDesc := cmd.BoolOpt("d sort-desc", false, "Sort descending (boolean, default to false)")
		order := cmd.StringOpt("order", "username", "Order results. Possible values are username, email, family name. Defaults to username")
		status := cmd.StringOpt("s status", "active", "Filter by account status of active (default), deactivated, and/or purged. Specify multiple with a comma-separated list")
		format := cmd.StringOpt("t format", "json", "One of json (default), csv, tsv, yaml, xml, ascii, markdown, html, mysql, postgres")

		cmd.Action = func() {
			c := newClient(*cfg)

			values := url.Values{}

			// validFilters := []string{"username", "user-id", "family-name", "email"}
			/*
				for _, f := range validFilters {
								if ctx.String(f) != "" {
									fu := strings.Replace(f, "-", "_", -1)
									values.Set(fu, ctx.String(f))
									if ctx.String("mode") == "prefix" {
										values.Set(fu+"_mode", ctx.String("mode"))
									}
								}
							} */

			if *limit > 0 {
				values.Set("limit", strconv.Itoa(*limit))
			}

			if *offset > 0 {
				values.Set("offset", strconv.Itoa(*offset))
			}

			if *order != "" {
				values.Set("order_by", *order)
			}

			if *status != "" {
				values.Set("status", *status)
			}

			if *sortDesc {
				values.Set("sort_descending", "true")
			}

			r, _ := c.NewRequest("GET", "/1/users?"+values.Encode())
			rsp := apiutil.DefaultErrorHandler(c.Do(r))
			data, err := rsp.Json()
			if err != nil {
				cliutil.Fail("Failed to decode JSON data: %v", err)
			}
			if rsp.StatusCode != http.StatusOK {
				apiutil.DefaultHandler(rsp, err)
			}

			if data["accounts"] != nil {
				accounts := data["accounts"].([]interface{})
				js, _ := json.Marshal(accounts)
				dataset, _ := tablib.LoadJSON(js)

				var s string
				switch *format {
				case "xml":
					s = dataset.XML()
				case "yaml":
					s, _ = dataset.YAML()
				case "csv":
					s, _ = dataset.CSV()
				case "tsv":
					s, _ = dataset.TSV()
				case "html":
					s = dataset.HTML()
				case "ascii":
					s = dataset.Tabular("condensed")
				case "markdown":
					s = dataset.Markdown()
				case "mysql":
					s = dataset.MySQL("Accounts")
				case "postgres":
					s = dataset.Postgres("Accounts")
				case "json":
					fallthrough
				case "":
					s, _ = dataset.JSON()
				default:
					cliutil.Fail("Unknown format: %s", *format)
				}
				fmt.Printf("%s\n", s)
			}
		}
	})
}
