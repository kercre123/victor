package token

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-jdocs/client/jdocs"
	"github.com/anki/sai-jdocs/proto/jdocspb"
	"github.com/anki/sai-service-framework/svc"
	"github.com/anki/sai-token-service/model"
	"github.com/jawher/mow.cli"
)

const (
	docname = "vic.AppTokens"
)

// ClientToken holds the tuple of the client token hash and the
// user-visible client name (e.g. Adam's iPhone)
type ClientToken struct {
	Hash       string `json:"hash"`
	ClientName string `json:"client_name"`
	AppId      string `json:"app_id"`
	IssuedAt   string `json:"issued_at"`
}

// ClientTokenDocument holds all the client token tuples for a given
// userid+robot, along with a handle to the Jdocs service document
// that stores them.
type ClientTokenDocument struct {
	ClientTokens []ClientToken `json:"client_tokens"`
	Jdoc         *jdocspb.Jdoc `json:"-"`
}

// ClientTokenStore is a configurable service object that manages
// storing hashed client tokens.
type ClientTokenStore interface {
	jdocs.Client

	// Retrieve fetches all the token hashes associated with the user
	// and robot identified by the access token.
	Retrieve(ctx context.Context, tok *model.Token) (*ClientTokenDocument, error)

	// Store adds a client token hash to the document store,
	// associated with the user and robot identified by the access
	// token.
	Store(ctx context.Context, hash string, tok *model.Token, clientName, appId string) error

	// Clear deletes all the client token hashes for the user and
	// robot identified by the access token.
	Clear(ctx context.Context, tok *model.Token) error
}

// ----------------------------------------------------------------------

type DummyClientTokenStore struct {
	jdocs.Client
}

func (s *DummyClientTokenStore) Retrieve(ctx context.Context, tok *model.Token) (*ClientTokenDocument, error) {
	return nil, nil
}

func (s *DummyClientTokenStore) Store(ctx context.Context, hash string, tok *model.Token, clientName, appId string) error {
	return nil
}
func (s *DummyClientTokenStore) Clear(ctx context.Context, tok *model.Token) error {
	return nil
}

// ----------------------------------------------------------------------

type jdocsClientTokenStore struct {
	jdocs.Client

	enabled *bool
}

func NewClientTokenStore() ClientTokenStore {
	enabled := false
	return &jdocsClientTokenStore{
		Client:  jdocs.NewClient(),
		enabled: &enabled,
	}
}

func (s *jdocsClientTokenStore) CommandSpec() string {
	return s.Client.CommandSpec() + " " +
		"[--client-token-store-enabled]"
}

func (s *jdocsClientTokenStore) CommandInitialize(cmd *cli.Cmd) {
	s.Client.CommandInitialize(cmd)
	s.enabled = svc.BoolOpt(cmd, "client-token-store-enabled", false,
		"Set to true to enable storing client tokens in the Jdocs service")
}

func (s *jdocsClientTokenStore) Start(ctx context.Context) error {
	if !*s.enabled {
		alog.Warn{
			"action":  "client_token_store_start",
			"enabled": *s.enabled,
			"msg":     "Client token store is disabled",
		}.Log()
		return nil
	}
	return s.Client.Start(ctx)
}

func (s *jdocsClientTokenStore) Retrieve(ctx context.Context, tok *model.Token) (*ClientTokenDocument, error) {
	if !*s.enabled {
		return nil, nil
	}

	// Set up the jdocs request to retrieve a single document
	req := &jdocspb.ReadDocsReq{
		UserId: tok.UserId,
		Thing:  tok.RequestorId,
		Items: []*jdocspb.ReadDocsReq_Item{
			{DocName: docname},
		},
	}

	// Call to jdocs
	resp, err := s.Client.ReadDocs(model.WithAccessToken(ctx, tok), req)
	if err != nil {
		return nil, err
	}

	// If there was no error, but the document doesn't exist
	if len(resp.Items) == 0 || (resp.Items[0].Status == jdocspb.ReadDocsResp_NOT_FOUND) {
		return nil, nil
	}

	// Otherwise, parse the document body
	var doc ClientTokenDocument
	err = json.Unmarshal([]byte(resp.Items[0].Doc.JsonDoc), &doc)
	if err != nil {
		return nil, err
	}

	doc.Jdoc = resp.Items[0].Doc
	return &doc, nil
}

func (s *jdocsClientTokenStore) Store(ctx context.Context, hash string, tok *model.Token, clientName, appId string) error {
	if !*s.enabled {
		return nil
	}

	// Fetch the doc for this user+robot
	doc, err := s.Retrieve(ctx, tok)
	if err != nil {
		return err
	}

	// if it doesn't exist, create a new one
	if doc == nil {
		doc = &ClientTokenDocument{
			ClientTokens: []ClientToken{},
			Jdoc: &jdocspb.Jdoc{
				FmtVersion: 1,
			},
		}
	}

	// Append the new hash tuple
	doc.ClientTokens = append(doc.ClientTokens, ClientToken{
		Hash:       hash,
		ClientName: clientName,
		AppId:      appId,
		IssuedAt:   time.Now().UTC().Format(time.RFC3339),
	})

	// Store the doc
	return s.write(ctx, doc, tok)
}

func (s *jdocsClientTokenStore) Clear(ctx context.Context, tok *model.Token) error {
	if !*s.enabled {
		return nil
	}

	// Fetch the doc for this user+robot
	doc, err := s.Retrieve(ctx, tok)
	if err != nil {
		return err
	}

	// If it doesn't exist, there's nothing to do
	if doc == nil {
		return nil
	}

	// If it does, empty the list
	doc.ClientTokens = doc.ClientTokens[:0]

	// Store the doc
	return s.write(ctx, doc, tok)
}

func (s *jdocsClientTokenStore) write(ctx context.Context, doc *ClientTokenDocument, tok *model.Token) error {
	// Serialize the body
	js, err := json.Marshal(doc)
	if err != nil {
		return err
	}
	doc.Jdoc.JsonDoc = string(js)

	// Update jdocs
	req := &jdocspb.WriteDocReq{
		UserId:  tok.UserId,
		Thing:   tok.RequestorId,
		DocName: docname,
		Doc:     doc.Jdoc,
	}
	resp, err := s.Client.WriteDoc(model.WithAccessToken(ctx, tok), req)
	if err != nil {
		return err
	}

	if resp.Status != jdocspb.WriteDocResp_ACCEPTED {
		return fmt.Errorf("Write rejected for %s/%s: %s/%d", docname, tok.RequestorId, resp.Status, resp.LatestDocVersion)
	}

	return nil
}
