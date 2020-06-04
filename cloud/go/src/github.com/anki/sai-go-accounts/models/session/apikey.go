// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package session

import "time"

type ApiKey struct {
	Id          string    `db:"apikey_id"`
	Token       string    `db:"apikey_token"`
	Scopes      ScopeMask `db:"scopes"`
	Description string    `db:"description"`
	TimeCreated time.Time `db:"time_created" json:"time_created"`
	TimeExpires time.Time `db:"time_expires" json:"time_expires"`
}
