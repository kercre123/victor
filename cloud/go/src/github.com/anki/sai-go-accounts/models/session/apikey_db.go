// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// +build !accounts_no_db

package session

import (
	"database/sql"
	"time"

	"github.com/anki/sai-go-accounts/db"
	"github.com/anki/sai-go-util/strutil"
	"github.com/coopernurse/gorp"
)

func init() {
	db.RegisterTable("apikeys", ApiKey{})
}

func (k *ApiKey) PreInsert(s gorp.SqlExecutor) (err error) {
	if err = db.AssignId(k); err != nil {
		return err
	}
	k.TimeCreated = time.Now()
	return nil
}

func NewApiKey(token string, scopes ScopeMask, description string, expires time.Time) (apiKey *ApiKey, err error) {
	if token == "" {
		if token, err = strutil.ShortMixedUUID(); err != nil {
			return nil, err
		}
	}
	k := &ApiKey{
		Token:       token,
		Scopes:      scopes,
		Description: description,
		TimeExpires: expires,
	}
	if err = db.Dbmap.Insert(k); err != nil {
		return nil, err
	}
	return k, nil
}

func ApiKeyByToken(apiKeyToken string) (k *ApiKey, err error) {
	// TODO: this should hit an in-proces cache 99% of the time
	k = &ApiKey{}
	err = db.Dbmap.SelectOne(k, "SELECT "+db.ColumnsFor["apikeys"]+" FROM apikeys WHERE apikey_token=?", apiKeyToken)
	if err == sql.ErrNoRows {
		return nil, nil
	}
	if err != nil {
		return nil, err
	}
	return k, nil
}
