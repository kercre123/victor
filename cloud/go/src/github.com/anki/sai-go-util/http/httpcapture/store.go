package httpcapture

import (
	"bytes"
	"compress/gzip"
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"path"
	"path/filepath"
	"strings"
)

var (
	DefaultHTTPClient = http.DefaultClient
)

// Store represents the interface for storing HAR archives.
type Store interface {
	StoreArchive(id string, data []byte, metadata Metadata) error
}

// FileStore stores HAR archives in a local directory.
type FileStore struct {
	Dir           string
	StoreMetadata bool
}

// NewFileStore creates a new FileStore instance.
func NewFileStore(dir string) (*FileStore, error) {
	fi, err := os.Stat(dir)
	if err != nil {
		return nil, fmt.Errorf("Invalid capture directory: %s", err)
	}
	if !fi.IsDir() {
		return nil, errors.New("Capture target is not a directory")
	}
	return &FileStore{Dir: dir}, nil
}

// StoreArchive implements the Store interface.
func (fs *FileStore) StoreArchive(id string, data []byte, metadata Metadata) error {
	fn := id + ".har"
	path := filepath.Join(fs.Dir, fn)
	f, err := os.Create(path)
	if err != nil {
		return err
	}
	defer f.Close()

	if _, err := f.Write(data); err != nil {
		return err
	}

	if fs.StoreMetadata {
		mfn := id + ".meta"
		mpath := filepath.Join(fs.Dir, mfn)
		mf, err := os.Create(mpath)
		if err != nil {
			return err
		}
		defer mf.Close()
		if err := json.NewEncoder(mf).Encode(metadata); err != nil {
			return err
		}
	}
	return nil
}

// BlobSTore uses the HTTP blob store service to store HAR archives.
type BlobStore struct {
	AppKey    string
	ServerURL string
	Namespace string
	client    *http.Client
}

// NewBlobStore creates a new BlobStore isntance.
// appKey supplies the Anki-App-Key to use to authenticate to the blob
// store service.
// url is the HTTP base URL for the service
// ns is the namespace to use for objects sent to the service.
// client is an optional HTTP client to use for upload requests.  It will
// defalut to the DefaultHTTPClient, or http.DefaultClient if not set.
func NewBlobStore(appKey, url, ns string, client *http.Client) (*BlobStore, error) {
	if appKey == "" {
		return nil, errors.New("appkey is required")
	}
	if url == "" {
		return nil, errors.New("blobstore url is required")
	}
	if ns == "" {
		return nil, errors.New("blobstore namespace is required")
	}

	if client == nil {
		client = DefaultHTTPClient
	}

	return &BlobStore{
		AppKey:    appKey,
		ServerURL: url,
		Namespace: ns,
		client:    client,
	}, nil
}

// StoreArchive stores the HAR archive in the blobstore
func (bs *BlobStore) StoreArchive(id string, data []byte, metadata Metadata) error {
	// As this is imported by apiclient, it cannot in turn use apiclient
	// (which also excludes it from using the blobstore client package)
	var gzd bytes.Buffer

	path := path.Join("1", bs.Namespace, "blobs", id)
	url := bs.ServerURL + "/" + path

	w := gzip.NewWriter(&gzd)
	if _, err := w.Write(data); err != nil {
		return err
	}

	if err := w.Close(); err != nil {
		return err
	}

	req, err := http.NewRequest("PUT", url, &gzd)
	if err != nil {
		return err
	}

	addMD := func(name, value string) {
		if value != "" {
			req.Header.Set(name, value)
		}
	}
	addMD("Anki-App-Key", bs.AppKey)
	addMD("Content-Type", "application/octet-stream")
	addMD("User-Agent", "Go-http-client/1.1 go-http-capture/1.0")
	addMD("Usr-RequestId", metadata.RequestId)
	addMD("Usr-RefRequestIds", strings.Join(metadata.ReferencedRequestIds, ","))
	addMD("Usr-ClientIP", metadata.ClientIP)
	addMD("Usr-HTTPURL", metadata.HTTPURL)
	addMD("Usr-ServerIP", metadata.ServerIP)
	addMD("Usr-Service", metadata.ServiceName)
	addMD("Usr-FileType", "har.gz")

	resp, err := bs.client.Do(req)
	if err != nil {
		return err
	}
	if resp.Body != nil {
		defer resp.Body.Close()
	}
	if resp.StatusCode < 200 || resp.StatusCode > 299 {
		body, _ := ioutil.ReadAll(resp.Body)
		return fmt.Errorf("Unexpected response for %s: %s - %s", path, resp.Status, string(body))
	}
	return nil
}

// StoreFromParams creates a store using the generic supplied parameters.
func StoreFromParams(appKey, storeType, storeTarget string) (Store, error) {
	switch storeType {
	case "file":
		return NewFileStore(storeTarget)

	case "blobstore":
		parts := strings.Split(storeTarget, "#")
		if len(parts) != 2 {
			return nil, fmt.Errorf("Illegal blobstore target %q", storeTarget)
		}
		return NewBlobStore(appKey, parts[0], parts[1], nil)

	default:
		return nil, errors.New("Unknown capture store type")
	}
}
