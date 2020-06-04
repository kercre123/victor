// Copyright 2016 Anki, Inc.

package apiutil

import (
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"

	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	api "github.com/anki/sai-go-util/http/apiclient"
)

const (
	Admin   = "admin"
	Support = "support"
	User    = "user"
	Service = "service"
	None    = ""

	ServiceAccounts = "accounts"
	ServiceAnkival  = "ankival"
	ServiceAudit    = "audit"
)

var (
	InvalidConfiguration = errors.New("Invalid client configuration")
)

func ApiClientCfg(cfg *config.Config, service config.Service) ([]api.Config, error) {
	url := cfg.Env.ServiceURLs.ByService(service)
	if url == "" {
		return nil, fmt.Errorf("No URL defined for service %v", service)
	}

	apiCfg := []api.Config{
		api.WithServerURL(url),
	}

	var debug int
	switch cfg.Common.DumpRequest {
	case "headers":
		debug |= api.RequestHeaders
	case "body":
		debug |= api.RequestBody
	case "both":
		debug |= api.RequestBody | api.RequestHeaders
	case "":
		// nop
	default:
		return nil, errors.New("Invalid value for dump request")
	}

	switch cfg.Common.DumpResponse {
	case "headers":
		debug |= api.ResponseHeaders
	case "body":
		debug |= api.ResponseBody
	case "both":
		debug |= api.ResponseBody | api.ResponseHeaders
	case "":
		// nop
	default:
		return nil, errors.New("Invalid value for dump request")
	}

	if debug > 0 {
		apiCfg = append(apiCfg, api.WithDebug(os.Stderr, debug))
	}

	if appkey := cfg.Session.AppKey; appkey != "" {
		apiCfg = append(apiCfg, api.WithAppKey(appkey))
	}

	if token := cfg.Session.Token; token != "" {
		apiCfg = append(apiCfg, api.WithSessionToken(token))
	}

	return apiCfg, nil
}

// ResponseError checks whether err or resp contains an error and returns
// a standard error response if so.
func ResponseError(resp *api.Response, err error) (*api.Response, error) {
	if err != nil {
		return nil, err
	}
	if resp.StatusCode == http.StatusOK {
		return resp, nil
	}
	body, _ := ioutil.ReadAll(resp.Body)
	return nil, fmt.Errorf("Response error (status=%d): %v", resp.StatusCode, body)
}

// DefaultHandler runs the default error handler, then parses the body
// as JSON and prints it, formatted and indented, to stdout.
func DefaultHandler(r *api.Response, err error) {
	data := DefaultJsonHandler(r, err)
	js, _ := json.MarshalIndent(data, "", "\t")
	fmt.Fprintf(os.Stdout, "%s\n", js)
}

// DefaultJsonHandler runs the default error handler, then parses and
// returns the response body as a generic Json datatype.
func DefaultJsonHandler(r *api.Response, err error) api.Json {
	if err != nil {
		cliutil.Fail("Error in request: %v", err)
	}
	data, err := r.Json()
	if err != nil {
		cliutil.Fail("Error parsing response: %v", err)
	}
	fmt.Fprintf(os.Stdout, "%#v\n", data)
	return data
}

// DefaultErrorHandler checks the error status of a response and exits
// if there was an error, or returns the response as a single value if
// there was no error.
func DefaultErrorHandler(r *api.Response, err error) *api.Response {
	if err != nil {
		cliutil.Fail("Error in request: %v", err)
	}
	return r
}

// GetStatus fetches the status from an API client, handles errors,
// and prints the status as JSON if there was no error.
func GetStatus(c *api.Client) {
	s, err := c.GetStatus()
	if err != nil {
		cliutil.Fail("Error in request: %v", err)
	}
	js, _ := json.MarshalIndent(s, "", "\t")
	fmt.Fprintf(os.Stdout, "%s\n", js)
}
