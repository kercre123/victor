package db

import (
	"context"
	"errors"
	"time"

	"github.com/anki/sai-awsutil/dynamoutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-token-service/model"
	"github.com/aws/aws-sdk-go/aws"
)

const (
	TableNameIssuedTokens        = "issued_jwt_tokens"
	TableNameRevokedTokens       = "revoked_jwt_tokens"
	TableNameRevokedCertificates = "revoked_certificates"

	IssuedTimeToLive  = time.Hour * 24 * 365
	RevokedTimeToLive = time.Hour * 24

	AttributeNameTokenId     = "token_id"
	AttributeNameTokenType   = "token_type"
	AttributeNameRequestorId = "requestor_id"
	AttributeNameUserId      = "user_id"
	AttributeNameAccountId   = "account_id"
	AttributeNameIssuedAt    = "issued_at"
	AttributeNameExpiresAt   = "expires_at"
	AttributeNameRevokedAt   = "revoked_at"
	AttributeNamePurgeAt     = "purge_at"
	AttributeNameRevoked     = "revoked"

	IndexNameByUserId      = "by_user_id"
	IndexNameByRequestorId = "by_requestor_id"
)

type DynamoTokenStore struct {
	db *dynamoutil.DB
}

func NewDynamoTokenStore(prefix string, cfg *aws.Config) (*DynamoTokenStore, error) {
	db, err := dynamoutil.NewDB(
		dynamoutil.WithPrefix(prefix+"_"),
		dynamoutil.WithConfig(cfg),

		//
		// Issued tokens tables. Tokens automatically expire out of
		// this table after one year, or are removed when explicitly
		// revoked.
		//
		dynamoutil.WithTable(&dynamoutil.TableSpec{
			Name: TableNameIssuedTokens,

			// Key by the token ID
			PrimaryKey: dynamoutil.KeySpec{
				KeyName: AttributeNameTokenId,
			},

			// Attributes used in indexes must be explicitly named
			Attributes: []dynamoutil.AttributeSpec{
				{Name: AttributeNameUserId},
				{Name: AttributeNameRequestorId},
				{Name: AttributeNameIssuedAt},
			},

			TimeToLiveAttribute: AttributeNamePurgeAt,

			GlobalIndexes: []dynamoutil.IndexSpec{
				{
					// Index by user ID for ListByUser, used to revoke
					// all tokens for a user.
					Name:       IndexNameByUserId,
					Projection: dynamoutil.ProjectAll,
					Key: dynamoutil.KeySpec{
						KeyName:    AttributeNameUserId,
						SubkeyName: AttributeNameIssuedAt,
						SubkeyType: dynamoutil.AttributeTypeString,
					},
				},
				{
					// Index by robot ID for ListByRequestor, used to
					// revoke all tokens for a thing.
					Name:       IndexNameByRequestorId,
					Projection: dynamoutil.ProjectAll,
					Key: dynamoutil.KeySpec{
						KeyName:    AttributeNameRequestorId,
						SubkeyName: AttributeNameIssuedAt,
						SubkeyType: dynamoutil.AttributeTypeString,
					},
				},
			},
		}),

		//
		// Revoked tokens table - items automatically expire from this
		// table after 24 hours.
		//
		dynamoutil.WithTable(&dynamoutil.TableSpec{
			Name: TableNameRevokedTokens,

			PrimaryKey: dynamoutil.KeySpec{
				KeyName: AttributeNameTokenId,
			},

			TimeToLiveAttribute: AttributeNamePurgeAt,
		}),
	)

	if err != nil {
		return nil, err
	}

	return &DynamoTokenStore{db: db}, nil
}

// ----------------------------------------------------------------------
// TokenStore interface
// ----------------------------------------------------------------------

var (
	ErrorTokenNotFound = errors.New("Token not found")
)

func (s *DynamoTokenStore) Fetch(ctx context.Context, id string) (*model.Token, error) {
	table := s.db.GetTable(TableNameIssuedTokens)
	key := toKey(&model.Token{Id: id})
	if item, err := table.SimpleGetItem(key); err != nil {
		return nil, err
	} else {
		if len(item) == 0 {
			return nil, ErrorTokenNotFound
		} else {
			return fromItem(item)
		}
	}
}

func (s *DynamoTokenStore) Store(ctx context.Context, t *model.Token) error {
	t.PurgeAt = t.IssuedAt.Add(IssuedTimeToLive).Truncate(time.Second).Round(0)
	err := s.db.GetTable(TableNameIssuedTokens).SimplePutItem(toItem(t))
	return err
}

func (s *DynamoTokenStore) Revoke(ctx context.Context, t *model.Token) error {
	t.Revoked = true
	t.RevokedAt = time.Now().UTC()
	t.PurgeAt = t.RevokedAt.Add(RevokedTimeToLive).Truncate(time.Second).Round(0)
	if err := s.db.GetTable(TableNameRevokedTokens).SimplePutItem(toItem(t)); err != nil {
		alog.Error{
			"action": "dynamo_revoke",
			"status": "error_storing_revoked",
			"error":  err,
		}.LogC(ctx)
		return err
	}
	if err := s.db.GetTable(TableNameIssuedTokens).SimpleDeleteItem(toKey(t)); err != nil {
		alog.Error{
			"action": "dynamo_revoke",
			"status": "error_deleting_issued",
			"error":  err,
		}.LogC(ctx)
		return err
	}
	return nil
}

func (s *DynamoTokenStore) ListTokens(ctx context.Context, searchBy TokenSearch, key string, prev *ResultPage) (*ResultPage, error) {
	table := s.db.GetTable(TableNameIssuedTokens)

	queryOpts := []dynamoutil.QueryConfig{}
	switch searchBy {
	case SearchByUser:
		queryOpts = append(queryOpts, dynamoutil.WithIndex(IndexNameByUserId))
	case SearchByRequestor:
		queryOpts = append(queryOpts, dynamoutil.WithIndex(IndexNameByRequestorId))
	}
	queryOpts = append(queryOpts, dynamoutil.WithSimpleKey(key))

	// TODO: implement paging
	out, err := table.Query(queryOpts...)
	if err != nil {
		return nil, err
	}

	page := &ResultPage{
		Tokens: []*model.Token{},
		Done:   true,
	}

	alog.Debug{
		"action":     "dynamo_list_tokens",
		"search_by":  searchBy,
		"key":        key,
		"item_count": len(out.Items),
	}.LogC(ctx)

	for _, item := range out.Items {
		if tok, err := fromItem(item); err != nil {
			return nil, err
		} else {
			page.Tokens = append(page.Tokens, tok)
		}
	}

	return page, nil
}

func (s *DynamoTokenStore) ListRevoked(ctx context.Context, prev *ResultPage) (*ResultPage, error) {
	table := s.db.GetTable(TableNameRevokedTokens)

	// TODO: implement paging
	scanOpts := []dynamoutil.ScanConfig{}
	out, err := table.Scan(scanOpts...)
	if err != nil {
		return nil, err
	}

	page := &ResultPage{
		Tokens: []*model.Token{},
		Done:   true,
	}

	alog.Debug{
		"action":     "dynamo_list_revoked",
		"item_count": len(out.Items),
	}.LogC(ctx)

	for _, item := range out.Items {
		if tok, err := fromItem(item); err != nil {
			return nil, err
		} else {
			page.Tokens = append(page.Tokens, tok)
		}
	}

	return page, nil
}

func (s *DynamoTokenStore) IsHealthy() error {
	return s.db.CheckStatus()
}

// ----------------------------------------------------------------------
// Dynamo helpers
// ----------------------------------------------------------------------

func (s *DynamoTokenStore) CreateTables() error {
	return s.db.CreateTables()
}

func (s *DynamoTokenStore) CheckTables() error {
	return s.db.CheckTables()
}

func (s *DynamoTokenStore) UpdateTTL() error {
	return s.db.UpdateTTL()
}

func toKey(t *model.Token) dynamoutil.Item {
	return dynamoutil.NewItem().
		SetStringValue(AttributeNameTokenId, t.Id)
}

func toItem(t *model.Token) dynamoutil.Item {
	return dynamoutil.NewItem().
		SetStringValue(AttributeNameTokenId, t.Id).
		SetStringValue(AttributeNameTokenType, t.Type).
		SetStringValueIfNotEmpty(AttributeNameRequestorId, t.RequestorId).
		SetStringValueIfNotEmpty(AttributeNameUserId, t.UserId).
		SetBoolValue(AttributeNameRevoked, t.Revoked).
		SetTimeValue(AttributeNameIssuedAt, t.IssuedAt).
		SetTimeValue(AttributeNameExpiresAt, t.ExpiresAt).
		SetTimeValue(AttributeNameRevokedAt, t.RevokedAt).
		SetUnixTimestampValue(AttributeNamePurgeAt, t.PurgeAt, time.Second)
}

func fromItem(item dynamoutil.Item) (*model.Token, error) {
	t := &model.Token{
		Id:          item.GetStringValue(AttributeNameTokenId),
		Type:        item.GetStringValue(AttributeNameTokenType),
		RequestorId: item.GetStringValue(AttributeNameRequestorId),
		UserId:      item.GetStringValue(AttributeNameUserId),
		Revoked:     item.GetBoolValue(AttributeNameRevoked),
	}
	if iat, err := item.GetTimeValue(AttributeNameIssuedAt); err != nil {
		return nil, err
	} else {
		t.IssuedAt = iat
	}
	if expires, err := item.GetTimeValue(AttributeNameExpiresAt); err != nil {
		return nil, err
	} else {
		t.ExpiresAt = expires
	}
	if revoked, err := item.GetTimeValue(AttributeNameRevokedAt); err != nil {
		return nil, err
	} else {
		t.RevokedAt = revoked
	}
	if purge, err := item.GetUnixTimestampValue(AttributeNamePurgeAt, time.Second); err != nil {
		return nil, err
	} else {
		t.PurgeAt = purge
	}
	return t, nil
}
