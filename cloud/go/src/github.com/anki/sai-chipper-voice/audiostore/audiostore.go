package audiostore

import (
	"bytes"
	"context"
	"errors"
	"strings"
	"time"

	"github.com/anki/sai-blobstore/client/blobstore"
	"github.com/anki/sai-go-util/http/apiclient"
	api "github.com/anki/sai-go-util/http/apiclient"
	"github.com/anki/sai-go-util/log"

	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
)

const (
	MaxDeviceIdLength = 128
	MaxSessionLength  = 128
)

var mdPrefix = "Usr-chipper-"

type Client struct {
	client    *blobstore.Client
	appKey    string
	namespace string
}

type Result struct {
	success bool
	id      string
}

// AudioBuffer interface. Blobstore client requires a ReadSeekCloser interface
type AudioBuffer struct {
	r *bytes.Reader
}

func (a AudioBuffer) Close() error {
	return nil
}

func (a AudioBuffer) Read(p []byte) (int, error) {
	return a.r.Read(p)
}

func (a AudioBuffer) Seek(offset int64, whence int) (int64, error) {
	return a.r.Seek(offset, whence)
}

func NewClient(prefix, namespace, appkey, storeUrl string) (*Client, error) {
	if appkey == "" || namespace == "" || storeUrl == "" {
		// trying to instantiate new client when not in dev
		return nil, errors.New("missing audiostore config")
	}

	c := &Client{
		appKey:    appkey,
		namespace: namespace,
	}

	bsClient, err := blobstore.New(prefix,
		apiclient.WithBuildInfoUserAgent(),
		apiclient.WithAppKey(appkey),
		apiclient.WithServerURL(storeUrl))

	if err != nil {
		return nil, err
	}
	c.client = bsClient
	return c, nil
}

type metadata struct {
	md map[string]string
}

func (m metadata) addIfNotEmpty(key, value string) {
	if value != "" {
		m.md[key] = value
	}
}

func (a *Client) Save(ctx context.Context, audio *bytes.Buffer, intent *pb.IntentResponse, encoding, service, requestId string, reqTime time.Time) Result {

	// create a ReadSeekCloser for audio
	r := AudioBuffer{bytes.NewReader(audio.Bytes())}

	deviceId := intent.DeviceId
	if len(deviceId) > MaxDeviceIdLength {
		deviceId = intent.DeviceId[:MaxDeviceIdLength]
		alog.Warn{"action": "truncate_device_id", "mode": "vc", "original": intent.DeviceId, "new": deviceId}.Log()
	}

	session := intent.Session
	if len(session) > MaxSessionLength {
		session = intent.Session[:MaxSessionLength]
		alog.Warn{"action": "truncate_session", "mode": "vc", "original": intent.Session, "new": session}.Log()
	}

	// metadata for searching
	m := metadata{md: make(map[string]string)}
	m.addIfNotEmpty(mdPrefix+"device-id", deviceId)
	m.addIfNotEmpty(mdPrefix+"session", session)
	m.addIfNotEmpty(mdPrefix+"intent", intent.IntentResult.Action)
	m.addIfNotEmpty(mdPrefix+"datetime-utc", FormatTime(reqTime))

	// unique-id, used for download
	blobId := BlobId(deviceId, session, encoding, service, reqTime)

	alog.Info{
		"action":     "save_audio",
		"request_id": requestId,
		"blob_id":    blobId,
		"size":       audio.Len(),
		"md":         m.md}.Log()

	// upload to blobstore and parse response
	uploadErr := jsonHandler(a.client.Upload(ctx, a.namespace, r, blobId, m.md))
	if uploadErr != nil {
		alog.Warn{
			"action":      "upload_audio",
			"status":      "unexpected_error",
			"mode":        "vc",
			"error":       uploadErr,
			"device":      deviceId,
			"session":     session,
			"device_org":  intent.DeviceId,
			"session_org": intent.Session,
			"request_id":  requestId,
		}.Log()
		return Result{false, ""}
	}

	return Result{true, blobId}
}

func (a *Client) SaveKG(ctx context.Context, audio *bytes.Buffer, kg *pb.KnowledgeGraphResponse, encoding, service, requestId string, reqTime time.Time) Result {

	// create a ReadSeekCloser for audio
	r := AudioBuffer{bytes.NewReader(audio.Bytes())}

	deviceId := kg.DeviceId
	if len(deviceId) > MaxDeviceIdLength {
		deviceId = kg.DeviceId[:MaxDeviceIdLength]
		alog.Warn{"action": "truncate_device_id", "mode": "kg", "original": kg.DeviceId, "new": deviceId}.Log()
	}

	session := kg.Session
	if len(session) > MaxSessionLength {
		session = kg.Session[:MaxSessionLength]
		alog.Warn{"action": "truncate_session", "mode": "kg", "original": kg.Session, "new": session}.Log()
	}

	// metadata for searching
	m := metadata{md: make(map[string]string)}
	m.addIfNotEmpty(mdPrefix+"device-id", deviceId)
	m.addIfNotEmpty(mdPrefix+"session", session)
	m.addIfNotEmpty(mdPrefix+"datetime-utc", FormatTime(reqTime))

	// unique-id, used for download
	blobId := BlobId(kg.DeviceId, kg.Session, encoding, service, reqTime)

	alog.Info{
		"action":     "save_audio",
		"request_id": requestId,
		"blob_id":    blobId,
		"size":       audio.Len(),
		"md":         m.md}.Log()

	// upload to blobstore and parse response
	uploadErr := jsonHandler(a.client.Upload(ctx, a.namespace, r, blobId, m.md))
	if uploadErr != nil {
		alog.Warn{
			"action":      "upload_audio",
			"status":      "unexpected_error",
			"mode":        "kg",
			"error":       uploadErr,
			"device":      deviceId,
			"session":     session,
			"device_org":  kg.DeviceId,
			"session_org": kg.Session,

			"request_id": requestId}.Log()
		return Result{false, ""}
	}

	return Result{true, blobId}
}

func FormatTime(dt time.Time) string {
	return dt.UTC().Format("2006-01-02T15:04:05Z")
}

func BlobId(deviceId, session, encoding, service string, dt time.Time) string {
	var enc string
	enc = "pcm"
	if encoding != "AUDIO_ENCODING_LINEAR_16" {
		enc = "opus"
	}
	return FormatTime(dt) + "-" + deviceId + "-" + session + "-" + enc + "-" + strings.ToLower(service) + ".raw"
}

func jsonHandler(r *api.Response, err error) error {
	if err != nil {
		return err
	}

	data, err := r.Json()
	if err != nil {
		return errors.New("json error: " + err.Error())
	}

	status, err := data.Field("status")
	if err != nil {
		return errors.New("missing status field in response")
	}

	if status != "ok" {
		return errors.New("status not ok")
	}

	return nil
}
