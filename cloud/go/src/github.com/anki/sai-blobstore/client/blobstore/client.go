package blobstore

import (
	"errors"
	"fmt"
	"io"
	"net/http"
	"strconv"
	"strings"
	"time"

	api "github.com/anki/sai-go-util/http/apiclient"
)

const (
	UploadTimeKey = "Srv-Upload-Time"
	ClientIPKey   = "Usr-Clientip"
	ServerIPKey   = "Usr-Serverip"
	HTTPURLKey    = "Usr-Httpurl"
)

type ReadSeekCloser interface {
	io.ReadSeeker
	io.Closer
}

// Client provides client API access to the blobstore service.
type Client struct {
	*api.Client
}

// New creates a new Client.
func New(prefix string, config ...api.Config) (*Client, error) {
	client, err := api.New(prefix, config...)
	if err != nil {
		return nil, err
	}
	return &Client{
		Client: client,
	}, nil
}

// Upload sends a raw file to the blobstore along with corresponding metadata.
// namespace holds the namespace to store the data in (eg. "cozmo")
// source contains the blob data to upload
// id supplies the name to assign to the blob
// metadata supplies the key:value metadata pairs.
func (c *Client) Upload(namespace string, source ReadSeekCloser, id string, metadata map[string]string) (*api.Response, error) {
	size, err := source.Seek(0, io.SeekEnd)
	if err != nil {
		return nil, err
	}
	if _, err := source.Seek(0, io.SeekStart); err != nil {
		return nil, err
	}

	r, err := c.NewRequest("PUT", "/1/"+namespace+"/blobs/"+id)
	if err != nil {
		return nil, err
	}

	for k, v := range metadata {
		r.Header.Set(k, v)
	}

	r.Header.Set("Content-Length", strconv.FormatInt(size, 10))
	r.Body = source

	return c.Do(r)
}

// Download retrieves a blob by id.
func (c *Client) Download(namespace, id string) (*api.Response, error) {
	r, err := c.NewRequest("GET", "/1/"+namespace+"/blobs/"+id)
	if err != nil {
		return nil, err
	}
	return c.Do(r)
}

// Info fetches metadata information about a blob.
func (c *Client) Info(namespace, id string) (*api.Response, error) {
	r, err := c.NewRequest("HEAD", "/1/"+namespace+"/blobs/"+id)
	if err != nil {
		return nil, err
	}
	return c.Do(r)
}

// FetchMeta fetches and decodes metadata information about a blob.
func (c *Client) FetchMeta(namespace, id string) (metadata map[string]string, err error) {
	resp, err := c.Info(namespace, id)
	if err != nil {
		return nil, fmt.Errorf("Failed to fetch metadata for id=%v: %v", id, err)
	}

	metadata = make(map[string]string)
	for k, v := range resp.Header {
		if !strings.HasPrefix(k, "Usr-") && !strings.HasPrefix(k, "Srv-") {
			continue
		}
		if len(v) > 0 {
			metadata[k] = v[0]
		}
	}
	return metadata, nil
}

// Tail polls the blobstore for new entries within a given namespace and calls
// a function after every poll supplying the zero or more matching entries retreived.
//
// It filters out duplicate entries and applies an optional filter supplied in TailConfig
// before passing the matches to f.
//
// If f returns true then Tail will exit.
func (c *Client) Tail(cfg *TailConfig, f func(matches []map[string]string) (stop bool)) error {
	if cfg.Namespace == "" {
		return errors.New("No namespace supplied")
	}

	if cfg.ScanBack == 0 {
		cfg.ScanBack = 30 * time.Second
	}

	if cfg.PollPeriod <= 0 {
		cfg.PollPeriod = 2 * time.Second
	}

	offset := time.Now().UTC().Add(-cfg.ScanBack).Format(time.RFC3339)

	// track entries seen from previous scan
	seen := make(map[string]bool)
	sp := SearchParams{
		Key:        UploadTimeKey,
		Value:      offset,
		SearchMode: "gte",
		MaxResults: 100,
	}

	for {
		//resp, err := c.Search(cfg.Namespace, sp)
		resp, err := c.Search(cfg.Namespace, sp)
		if err != nil {
			return err
		}
		if resp.StatusCode != http.StatusOK {
			return fmt.Errorf("Non 200 response received: %v", resp.Status)
		}

		data, err := resp.Json()
		if err != nil {
			return fmt.Errorf("Failed to decode JSON: %v", err)
		}

		results, ok := data["results"].([]interface{})
		if !ok {
			fmt.Errorf("No results in data %v", data)
		}

		newseen := make(map[string]bool)
		matches := make([]map[string]string, 0, len(results))
		oldoffset := offset
		for _, result := range results {
			kvs, ok := result.(map[string]interface{})
			if !ok {
				return fmt.Errorf("Unexpected entry in results %v", kvs)
			}
			// set new time offset
			offset = kvs["mdvalue"].(string)
			id := kvs["id"].(string)
			newseen[id] = true
			if seen[id] {
				continue
			}

			md, err := c.FetchMeta(cfg.Namespace, id)
			if err != nil {
				return err
			}
			if !cfg.Filters.IsFiltered(md) {
				matches = append(matches, md)
			}
		}
		if oldoffset != offset {
			seen = newseen
		}

		if stop := f(matches); stop {
			break
		}

		time.Sleep(cfg.PollPeriod)
	}
	return nil
}

// SearchParams defines search parameters for use with Search.
type SearchParams struct {
	Key        string `json:"key"`
	Value      string `json:"value"`
	Value2     string `json:"value2"`
	SearchMode string `json:"mode"`
	MaxResults int    `json:"limit"`
}

// Search executes a metadata search for blobs using the supplied parameters
func (c *Client) Search(namespace string, search SearchParams) (*api.Response, error) {
	r, err := c.NewRequest("POST", "/1/"+namespace+"/search",
		api.WithJsonBody(search))
	if err != nil {
		return nil, err
	}
	return c.Do(r)
}

// TailConfig defines configuration for use with BlobstoreClient.Tail.
type TailConfig struct {
	Namespace  string          // The namespace to tail
	ScanBack   time.Duration   // how far to scan back in time; defaults to 5 seconds
	PollPeriod time.Duration   // how often to poll the server; defaults to 2 seconds
	Filters    *MetadataFilter // optional set of filters to apply
}

// FilterMatches deifnes An individual filter test; should return true if the supplied
// metadata passes the filter (ie. should not be rejected).
type FilterMatcher func(md map[string]string) bool

type MetadataFilter struct {
	Filters []FilterMatcher
}

// IsFiltered returns true if the supplied metadata should be
// filtered out (ie. rejected)
func (f *MetadataFilter) IsFiltered(md map[string]string) bool {
	if f == nil {
		return false
	}
	for _, matcher := range f.Filters {
		if !matcher(md) {
			return true
		}
	}
	return false
}

// AddExact defines a filter that must exactly match the supplied
// header with a value (case insensitive)
func (f *MetadataFilter) AddExact(header, value string) {
	if value == "" {
		return
	}
	lcvalue := strings.ToLower(value)
	f.Filters = append(f.Filters, func(md map[string]string) bool {
		return strings.ToLower(md[header]) == lcvalue
	})
}

// AddPrefix defines a filter that matches a header value
// against a given prefix (case insensitive)
func (f *MetadataFilter) AddPrefix(header, value string) {
	if value == "" {
		return
	}
	lcvalue := strings.ToLower(value)
	f.Filters = append(f.Filters, func(md map[string]string) bool {
		return strings.HasPrefix(strings.ToLower(md[header]), lcvalue)
	})
}

// AddFilter adds an arbitrary FilterMatcher function to a chain
// of filters.
func (f *MetadataFilter) AddFilter(fm FilterMatcher) {
	f.Filters = append(f.Filters, fm)
}
