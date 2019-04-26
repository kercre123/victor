package blobstore

import (
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path"
	"regexp"
	"strconv"
	"strings"

	"github.com/anki/sai-blobstore/apierrors"
	"github.com/anki/sai-blobstore/db"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/service/s3"
)

const (
	maxItemSize   = 500 * 1000 * 1000 // 500MB
	mdContentSize = "Srv-Blob-Size"
)

var (
	ErrItemTooLarge = errors.New("Blob too large")
)

type S3ReadWriter interface {
	GetObject(input *s3.GetObjectInput) (*s3.GetObjectOutput, error)
	PutObject(input *s3.PutObjectInput) (*s3.PutObjectOutput, error)
}

type BlobData struct {
	Body            io.ReadCloser
	Size            int64
	ContentType     string
	ContentEncoding string
}

type Store struct {
	S3     S3ReadWriter
	Bucket string
	Prefix string
}

// StoreItem reads binary data from a reader, stores it in S3
// and commits the metadata to Dynamo
func (bs *Store) StoreItem(ns, id string, data io.Reader, metadata map[string]string) (size int64, err error) {
	ns, err = normalizeNamespace(ns)
	if err != nil {
		return 0, err
	}

	// buffer data to disk as S3 requires a seekable reader
	tf, err := ioutil.TempFile("", "blobstore-")
	if err != nil {
		return 0, err
	}
	defer func() {
		tf.Close()
		os.Remove(tf.Name())
	}()

	size, err = io.Copy(tf, io.LimitReader(data, maxItemSize))
	if err != nil {
		return 0, err
	}
	if size >= maxItemSize {
		return 0, ErrItemTooLarge
	}

	metadata[mdContentSize] = strconv.FormatInt(size, 10)

	// Seek to beginning of tempfile for upload to S3
	tf.Seek(0, 0)

	// Do the upload
	nsid := addNsToId(ns, id)
	req := &s3.PutObjectInput{
		Body:   tf,
		Bucket: aws.String(bs.Bucket),
		Key:    aws.String(path.Join(bs.Prefix, nsid)),
	}

	if _, err := bs.S3.PutObject(req); err != nil {
		return 0, fmt.Errorf("PUT to S3 failed: %v", err)
	}

	// Update Dynamo
	nsmd := addNsToMetadata(ns, metadata)
	if err := db.SaveBlobMetadata(nsid, nsmd); err != nil {
		return 0, fmt.Errorf("Failed to save metadata to Dynamo: %v", err)
	}
	return size, nil
}

// Fetch the binary data from S3
func (bs *Store) FetchItemData(ns, id string) (blob *BlobData, err error) {
	ns, err = normalizeNamespace(ns)
	if err != nil {
		return nil, err
	}

	nsid := addNsToId(ns, id)
	key := path.Join(bs.Prefix, nsid)
	req := &s3.GetObjectInput{
		Bucket: aws.String(bs.Bucket),
		Key:    aws.String(key),
	}
	resp, err := bs.S3.GetObject(req)
	if err != nil {
		return nil, fmt.Errorf("Failed to fetch object %s from S3: %v", key, err)
	}

	blob = &BlobData{
		Body:            resp.Body,
		Size:            aws.Int64Value(resp.ContentLength),
		ContentType:     aws.StringValue(resp.ContentType),
		ContentEncoding: aws.StringValue(resp.ContentEncoding),
	}
	return blob, nil
}

// Fetch the blob's metadata from Dynamo
func (bs *Store) FetchItemMetadata(ns, id string) (metadata map[string]string, err error) {
	ns, err = normalizeNamespace(ns)
	if err != nil {
		return nil, err
	}

	nsid := addNsToId(ns, id)
	metadata, err = db.ByID(nsid)
	return stripNsFromMetadata(ns, metadata), err
}

func (bs *Store) FindItems(ns, key, value1, value2 string, searchType db.SearchMode) ([]db.Record, error) {
	key = addNsToMetadataKey(ns, key)
	result, err := db.ByMetadataKey(key, value1, value2, searchType)
	if err != nil {
		if aerr, ok := err.(awserr.Error); ok {
			switch aerr.Code() {
			case "ValidationException":
				return nil, apierrors.NewValidationError(aerr.Message())
			}
		}
		return nil, err
	}

	for i, rec := range result {
		result[i].Id = stripNsFromId(ns, rec.Id)
		result[i].MetadataKey = stripNsFromMetadataKey(ns, rec.MetadataKey)
	}

	return result, err
}

var nsRegexp = regexp.MustCompile("^[a-z0-9-]+$")

func normalizeNamespace(ns string) (string, error) {
	ns = strings.ToLower(ns)
	if !nsRegexp.MatchString(ns) {
		return "", apierrors.InvalidNamespace
	}
	return ns, nil
}

func addNsToId(ns, id string) string {
	return path.Join(ns, id)
}

func stripNsFromId(ns, id string) string {
	return id[len(ns)+1:]
}

func addNsToMetadataKey(ns, k string) string {
	return ns + "-" + k
}

func stripNsFromMetadataKey(ns, k string) string {
	return k[len(ns)+1:]
}

func addNsToMetadata(ns string, md map[string]string) (result map[string]string) {
	result = make(map[string]string, len(md))
	for k, v := range md {
		result[ns+"-"+k] = v
	}
	return result
}

func stripNsFromMetadata(ns string, md map[string]string) (result map[string]string) {
	result = make(map[string]string, len(md))
	for k, v := range md {
		result[k[len(ns)+1:]] = v
	}
	return result
}
