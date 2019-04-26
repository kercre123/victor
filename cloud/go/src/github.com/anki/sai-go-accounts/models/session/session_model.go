// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package session

import (
	"time"

	"github.com/anki/sai-go-accounts/models/user"
)

type Session struct {
	Id          string    `db:"session_id" json:"session_token"`
	UserId      string    `db:"user_id" json:"user_id"`               // "" for admin accounts
	RemoteId    string    `db:"remote_id" json:"remote_id,omitempty"` // Used only for admin accounts
	Scope       Scope     `db:"scope" json:"scope"`
	TimeCreated time.Time `db:"time_created" json:"time_created"`
	TimeExpires time.Time `db:"time_expires" json:"time_expires"`
}

type UserSession struct {
	Session *Session          `json:"session"`
	User    *user.UserAccount `json:"user,omitempty"`
	// TODO add admin in here too
}

func (s *Session) HasExpired() bool {
	return s.TimeExpires.Before(time.Now())
}

func (s *Session) Token() string {
	return s.Id
}
