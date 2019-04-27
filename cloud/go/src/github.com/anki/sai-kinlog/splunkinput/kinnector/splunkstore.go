// Copyright 2016 Anki, Inc.

package kinnector

import (
	"bytes"
	"crypto/tls"
	"encoding/json"
	"fmt"
	"io"
	"net"
	"net/http"
	"net/url"
	"strings"
	"time"

	"github.com/anki/sai-kinlog/kinreader"
)

// SplunkError holds an error returned from a Splunk API.
type SplunkError struct {
	StatusCode int `json:"status_code"`
	Messages   []struct {
		Type string `json:"type"`
		Text string `json:"text"`
	} `json:"messages"`
}

// Error fulfills the Error interface.
func (se SplunkError) Error() string {
	if len(se.Messages) > 0 {
		msg := se.Messages[0]
		return fmt.Sprintf("%s (%d): %s", msg.Type, se.StatusCode, msg.Text)
	}
	return "Invalid Splunk Error"
}

// SplunkKVCheckpointer implements the kinreader.Checkpointer interface for storing
// and retrieving  checkpoint information in the Splunk KV store.
type SplunkKVCheckpointer struct {
	ServerURI  string
	SessionKey string
	AppName    string
	TableName  string
	client     *http.Client
}

// NewSplunkKVCheckpointer create and initializes a SplunkKVInstance.
// It will attempt to connect to the Splunk KV service and create the requested
// table (if necessary) prior to returning.
func NewSplunkKVCheckpointer(serverURI, sessionKey, appName, tableName string) (*SplunkKVCheckpointer, error) {
	cp := &SplunkKVCheckpointer{
		ServerURI:  serverURI,
		SessionKey: sessionKey,
		AppName:    appName,
		TableName:  tableName,
		client: &http.Client{
			Transport: &http.Transport{
				Proxy: http.ProxyFromEnvironment,
				Dial: (&net.Dialer{
					Timeout:   30 * time.Second,
					KeepAlive: 30 * time.Second,
				}).Dial,
				TLSHandshakeTimeout: 10 * time.Second,
				TLSClientConfig: &tls.Config{
					InsecureSkipVerify: true,
				},
			},
		},
	}

	resp, err := cp.doFormPost("/config", url.Values{
		"name": {tableName},
	})
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	if resp.StatusCode == http.StatusConflict {
		return cp, nil // already exists; assume ok
	}

	if err := cp.parseResponseError(resp); err != nil {
		return nil, err
	}
	return cp, nil
}

// AllCheckpoints retrieves all current checkpoints from the KV store.
func (cp *SplunkKVCheckpointer) AllCheckpoints() (kinreader.Checkpoints, error) {
	var data []kinreader.Checkpoint

	err := cp.doGet("/data/"+cp.TableName, &data)
	if err != nil {
		return nil, err
	}

	// convert to map
	result := make(kinreader.Checkpoints)
	for i, cp := range data {
		result[cp.ShardId] = &data[i]
	}
	return result, nil

}

// SetCheckpoint stores a checkpoint in the KV store.
func (cp *SplunkKVCheckpointer) SetCheckpoint(c kinreader.Checkpoint) error {
	// generate a new document and store it; in future this will have to check for races
	// against concurrent stream processors.
	c.Time = time.Now()
	data := checkPoint{Checkpoint: c, Key: c.ShardId}
	return cp.createOrUpdate(c.ShardId, data)
}

func (cp *SplunkKVCheckpointer) newRequest(method, endpoint string, body io.Reader) *http.Request {
	uri := fmt.Sprintf("%s/servicesNS/nobody/%s/storage/collections%s?output_mode=json",
		cp.ServerURI, cp.AppName, endpoint)
	r, _ := http.NewRequest(method, uri, body)
	r.Header.Add("Authorization", "Splunk "+cp.SessionKey)
	return r

}

func (cp *SplunkKVCheckpointer) doGet(endpoint string, target interface{}) error {
	resp, err := cp.client.Do(cp.newRequest("GET", endpoint, nil))
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	switch resp.StatusCode {
	case http.StatusConflict, http.StatusOK:
		return json.NewDecoder(resp.Body).Decode(&target)
	default:
		return cp.parseResponseError(resp)
	}
}

func (cp *SplunkKVCheckpointer) doJSONPost(endpoint string, data interface{}) (*http.Response, error) {
	b, _ := json.Marshal(data)
	req := cp.newRequest("POST", endpoint, bytes.NewBuffer(b))
	req.Header.Set("Content-Type", "application/json")
	return cp.client.Do(req)
}

func (cp *SplunkKVCheckpointer) doFormPost(endpoint string, data url.Values) (*http.Response, error) {
	req := cp.newRequest("POST", endpoint, strings.NewReader(data.Encode()))
	req.Header.Set("Content-Type", "application/x-www-form-urlencoded")
	return cp.client.Do(req)
}

func (cp *SplunkKVCheckpointer) createOrUpdate(key string, doc interface{}) (err error) {
	var resp *http.Response
	for i := 0; i < 2; i++ {
		// attempt update first as that's the usual case
		resp, err = cp.doJSONPost(fmt.Sprintf("/data/%s/%s", cp.TableName, key), doc)
		if err != nil {
			return err
		}
		// Update will return not found if the doc isn't there to create
		// try to insert it
		if resp.StatusCode != http.StatusNotFound {
			// either the update went ok, or an error occured; return either way
			return cp.parseResponseError(resp)
		}

		// probably doesn't exist; try to create it
		resp, err = cp.doJSONPost("/data/"+cp.TableName, doc)
		if err != nil {
			return err
		}
		// if the insert goes ok then exit
		// if there's a conflict, then another writer beat us to the insert and
		// we need to repeat the update
		if resp.StatusCode != http.StatusConflict && (resp.StatusCode < 200 || resp.StatusCode > 299) {
			return cp.parseResponseError(resp)
		}

		// Had a conflict response indicating we raced against another writer;
		// try to update again
	}
	return cp.parseResponseError(resp)
}

func (cp *SplunkKVCheckpointer) parseResponseError(resp *http.Response) error {
	if resp.StatusCode >= 200 && resp.StatusCode <= 299 {
		return nil
	}

	var errs SplunkError
	if err := json.NewDecoder(resp.Body).Decode(&errs); err != nil {
		return err
	}
	errs.StatusCode = resp.StatusCode
	return errs
}

type checkPoint struct {
	kinreader.Checkpoint
	Key string `json:"_key"`
}
