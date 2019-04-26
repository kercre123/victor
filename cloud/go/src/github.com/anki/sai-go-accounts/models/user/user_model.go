// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// The user package manages the database models for user accounts
package user

import (
	"database/sql/driver"
	"errors"
	"fmt"
	"strings"
	"time"

	"github.com/anki/sai-go-util/date"
	"github.com/anki/sai-go-util/timedtoken"
)

var (
	NoDOBError = errors.New("No DOB for account")
)

// The AccountStatus type maps between the single character representation
// of the status that's stored in the DB ("A", "D", "P") and the value
// that sent/received over JSON ("active", "deleted", "purged").
// The string actually held in the AccountStatus string is the DB value.
type AccountStatus string

// Marshal the value into the string to be transmitted by JSON
func (s AccountStatus) MarshalText() ([]byte, error) {
	switch s {
	case AccountActive:
		return []byte("active"), nil
	case AccountDeactivated:
		return []byte("deleted"), nil
	case AccountPurged:
		return []byte("purged"), nil
	default:
		return []byte("unknown"), nil
	}
}

// Convert from the JSON value back to the DB value.
func (s *AccountStatus) UnmarshalText(text []byte) error {

	switch strings.ToLower(string(text)) {
	case "active":
		*s = AccountActive
	case "deleted":
		*s = AccountDeactivated
	case "purged":
		*s = AccountPurged
	default:
		return fmt.Errorf("Invalid AccountStatus %q", string(text))
	}
	return nil
}

// Type conversion so the DB knows how to store the value.
func (s AccountStatus) Value() (driver.Value, error) {
	return driver.Value(string(s)), nil
}

// Tyep conversion so the DB can read the value.
func (s *AccountStatus) Scan(src interface{}) error {
	*s = AccountStatus(src.([]byte))
	return nil
}

const (
	AccountActive      AccountStatus = "A"
	AccountDeactivated AccountStatus = "D"
	AccountPurged      AccountStatus = "P"
	GenderMale                       = "M"
	GenderFemale                     = "F"
	GenderDeclined                   = "-"
)

type DeactivationNoticeState int

const (
	NoDeactivationNoticeSent     DeactivationNoticeState = 0
	FirstDeactivationNoticeSent                          = 1
	SecondDeactivationNoticeSent                         = 2
)

// Limits
const (
	UsernameMinLength     = 3
	UsernameMaxLength     = 12
	EmailMaxLength        = 100
	FamilyNameMinLength   = 0
	FamilyNameMaxLength   = 100
	GivenNameMinLength    = 00
	GivenNameMaxLength    = 100
	PasswordMinLength     = 6
	PasswordMaxLength     = 100
	DriveGuestIdMinLength = 0
	DriveGuestIdMaxLength = 40
)

// EmiledTokensLifetime
const (
	EmailVerifyLifetime = time.Hour * 24 * 500 // TODO: Temporary increase during bulk notification - Switch back to 30 day window by 2016-01-01
	//EmailVerifyLifetime   = time.Hour * 24 * 30
	PasswordResetLifetime = time.Hour * 24 * 2
)

var (
	AccountValidStates = []AccountStatus{AccountActive, AccountDeactivated, AccountPurged}
	ValidGenders       = []string{GenderMale, GenderFemale, GenderDeclined}
	/*
		DuplicateUsername       = errors.New("Duplicate username")
		AlreadyLinked           = errors.New("Linked account is already in use with another account")
		UserNotFound            = errors.New("User not found")
	*/
)

// BlockedEmail holds a permanently blocked email address; the system
// will not allows accounts to be created, or for emails to be sent to
// blocked addresses.
type BlockedEmail struct {
	Email       string    `db:"email" json:"email" dbflags:"prikey"`
	TimeBlocked time.Time `db:"time_blocked" json:"time_blocked"`
	BlockedBy   string    `db:"blocked_by" json:"blocked_by"`
	Reason      string    `db:"reason" json:"reason"`
}

// User holds a user's profile information.
//
// See UserAccount for the structure that also combines any LinkedAccounts
type User struct {
	// NOTE: If you add any Personally Identifiable Information (PII) to this struct,
	// be sure to also add it to PurgeDeactivated() and Purge() calls
	Id                           string                  `db:"user_id" json:"user_id"`
	DriveGuestId                 *string                 `db:"drive_guest_id" json:"drive_guest_id"`
	PlayerId                     *string                 `db:"-" json:"player_id"` // a read-only alias of DriveGuestID - SAIP-1004
	CreatedByAppName             *string                 `db:"created_by_app_name" json:"created_by_app_name"`
	CreatedByAppVersion          *string                 `db:"created_by_app_version" json:"created_by_app_version"`
	CreatedByAppPlatform         *string                 `db:"created_by_app_platform" json:"created_by_app_platform"`
	DOB                          date.Date               `db:"dob" json:"dob"`
	Email                        *string                 `db:"email" json:"email"`
	FamilyName                   *string                 `db:"family_name" json:"family_name"`
	Gender                       *string                 `db:"gender" json:"gender"`
	GivenName                    *string                 `db:"given_name" json:"given_name"`
	Username                     *string                 `db:"username" json:"username"`
	DeactivatedUsername          *string                 `db:"deactivated_username" json:"deactivated_username,omitempty"`
	EmailIsVerified              bool                    `db:"email_is_verified" json:"email_is_verified"`
	EmailFailureCode             *string                 `db:"email_failure_code" json:"email_failure_code"`
	EmailLang                    *string                 `db:"email_lang" json:"email_lang"`
	PasswordIsComplex            bool                    `db:"password_is_complex" json:"password_is_complex"`
	Status                       AccountStatus           `db:"status" json:"status"`
	TimeCreated                  time.Time               `db:"time_created" json:"time_created"`
	TimeDeactivated              *time.Time              `db:"time_deactivated" json:"-"`
	TimePurged                   *time.Time              `db:"time_purged" json:"-"`
	TimeUpdated                  *time.Time              `db:"time_updated" json:"-"`
	TimePasswordUpdated          *time.Time              `db:"time_password_updated" json:"-"`
	TimeEmailFirstVerified       *time.Time              `db:"time_email_first_verified" json:"-"`
	PasswordHash                 HashedPassword          `db:"password_hash" json:"-"`
	PasswordReset                *string                 `db:"password_reset" json:"-"`
	EmailVerify                  *string                 `db:"email_verify" json:"-"`
	DeactivationNoticeState      DeactivationNoticeState `db:"deactivation_notice_state" json:"-"`
	TimeDeactivationStateUpdated *time.Time              `db:"time_deactivation_state_updated" json:"-"`
	DeactivationReason           *string                 `db:"deactivation_reason" json:"deactivation_reason"`
	PurgeReason                  *string                 `db:"purge_reason" json:"purge_reason"`
	EmailIsBlocked               bool                    `db:"-" json:"email_is_blocked"`
	NoAutodelete                 bool                    `db:"no_autodelete" json:"no_autodelete"`
}

type UserAccount struct {
	*User
	NewPassword *string `json:"password,omitempty"`
}

// IsEmailVerify takes an email verify timedtoken from the api and confirmes that
// for this use this token is valid, both connected to this user and not expired
func (ua *UserAccount) IsEmailVerify(evtimedtoken string) bool {
	u := ua.User
	if u == nil || u.EmailVerify == nil || evtimedtoken == "" {
		return false
	}
	if *u.EmailVerify != evtimedtoken {
		return false
	}
	return timedtoken.Valid(evtimedtoken, EmailVerifyLifetime)
}

// IsValidPasswordToken takes a password reset timedtoken from the api and confirms that
// for this use this token is valid, both connected to this user and not expired
func (ua *UserAccount) IsValidPasswordToken(evtimedtoken string) bool {
	u := ua.User
	if u == nil || u.PasswordReset == nil || evtimedtoken == "" {
		return false
	}
	if *u.PasswordReset != evtimedtoken {
		return false
	}
	return timedtoken.Valid(evtimedtoken, PasswordResetLifetime)
}

// EmailLocaleCountry extracts the user's locale country based on their
// EmailLang preference.  It returns a lower case string, or an empty
// string if the locale is not set.
//
// For example, this returns "us" from "en-US"
func (ua *UserAccount) EmailLocaleCountry() string {
	if ua.EmailLang == nil || len(*ua.EmailLang) != 5 {
		return ""
	}
	return strings.ToLower((*ua.EmailLang)[3:])
}

// FilterField is a model for filtering users
// Name is the name of the field to filter on (from the User struct)
// Value is passed by the user
// Mode is PrefixFilter or MatchFilter
type FilterField struct {
	Name  string
	Value string
	Mode  string
}

// Modifiers that can be applied when listing accounts
type AccountListModifier struct {
	Status   []AccountStatus
	Filters  []FilterField
	Limit    int
	OrderBy  string
	Offset   int
	SortDesc bool
}

const (
	UserId       string = "user_id"
	Email        string = "email"
	Username     string = "username"
	FamilyName   string = "family_name"
	Active       string = "active"
	Deactivated  string = "deactivated"
	Purged       string = "purged"
	PrefixFilter string = "prefix"
	MatchFilter  string = "match"
	MaxListLimit        = 100
)

// IsChild returns a boolean if the user is defined as a legal child in their locale.
//
// Locale is determined by their email preference at the current time.
// See SAIP-1542.
func (ua *UserAccount) IsChild() (bool, error) {
	consentThreshold := 13

	if ua == nil {
		return false, errors.New("Nil user account")
	}

	if ua.DOB.IsZero() {
		return false, NoDOBError
	}

	switch country := ua.EmailLocaleCountry(); country {
	case "gb", "us":
		consentThreshold = 13

	case "fr":
		consentThreshold = 15

	// Remaining EU GDPR member states
	case "at", "be", "bg", "cy", "cz", "de", "dk", "ee", "es", "fi", "gr",
		"hr", "hu", "ie", "it", "lt", "lv", "lu", "mt", "nl", "pl", "pt",
		"ro", "se", "si", "sk":
		consentThreshold = 16

	default:
		// rest of world defaults to 13
		consentThreshold = 13
	}

	return !ua.DOB.AddDate(consentThreshold, 0, 0).AddDate(0, 0, -1).Before(date.Today()), nil
}
