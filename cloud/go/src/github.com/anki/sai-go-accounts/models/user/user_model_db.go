// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// +build !accounts_no_db

// The user package manages the database models for user accounts
package user

import (
	"bytes"
	"compress/gzip"
	"database/sql"
	"encoding/csv"
	"errors"
	"fmt"
	"io"
	"math"
	"net/http"
	"net/mail"
	"net/url"
	"regexp"
	"strconv"
	"strings"
	"time"
	"unicode"
	"unicode/utf8"

	"github.com/anki/sai-go-accounts/apierrors"
	"github.com/anki/sai-go-accounts/db"
	"github.com/anki/sai-go-accounts/email"
	autil "github.com/anki/sai-go-accounts/util"
	"github.com/anki/sai-go-util/date"
	"github.com/anki/sai-go-util/jsonutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/postmarkapp"
	"github.com/anki/sai-go-util/task"
	"github.com/anki/sai-go-util/timedtoken"
	"github.com/anki/sai-go-util/validate"
	"github.com/coopernurse/gorp"
	uuid "github.com/satori/go.uuid"
)

var (
	// Hour long a user has to confirm their email address before their
	// account is considered inactive and their account is deactivated
	// See SAIAC-216, SAIAC-217
	userConfirmationTimeout = 15 * 24 * time.Hour

	// How long after an account is marked as deactivated before all PII is purged
	// from it
	deactivatePurgeTimeout = 30 * 24 * time.Hour

	// Send first deactivation notice period 7 days before userConfirmationTimeout
	deactivateNotifyPeriodOne = 7 * 24 * time.Hour

	// Send second deactivation notice period 6 days after the first notice was sent
	deactivateNotifyPeriodTwo = 6 * 24 * time.Hour

	// Perform actual deactivation 24 hours after the final notice is sent out
	deactivationPeriodThree = 24 * time.Hour

	// emailLangRegexp used to validate Account email_lang parameter
	emailLangRegexp = regexp.MustCompile("^[a-z][a-z]-[A-Z][A-Z]$")
)

var (
	InvalidPasswordToken = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "invalid_password_reset_token",
		Message:    "Invalid password reset token",
	}
	InvalidEmail = &jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "invalid_email_address",
		Message:    "Email address is marked as invalid",
	}
)

var (
	maxEmailsPerBatch     = 250
	maxUsersPerDeactivate = 500
	maxUsersPerPurge      = 500
)

const (
	deactivationTimeoutReason = "non-activation-timeout-reached"
	purgeTimeoutReason        = "purge-timeout-reached"
)

func init() {
	db.RegisterTable("users", User{})
	db.RegisterTable("blocked_emails", BlockedEmail{})
}

func (u *User) PreInsert(s gorp.SqlExecutor) (err error) {
	if err = db.AssignId(u); err != nil {
		return err
	}
	u.TimeCreated = time.Now()

	// Ensure all new users are given a guest/player id if not explicitly allocated
	// See SAIP-1011
	if u.DriveGuestId == nil || *u.DriveGuestId == "" {
		id := uuid.NewV4().String()
		u.DriveGuestId = &id
	}
	u.PlayerId = u.DriveGuestId
	return nil
}

func (u *User) PreUpdate(s gorp.SqlExecutor) (err error) {
	now := time.Now()
	u.TimeUpdated = &now
	u.PlayerId = u.DriveGuestId
	return nil
}

func (u *User) PostGet(s gorp.SqlExecutor) (err error) {
	// PlayerId is an alias of DriveGuestId - SAIP-1004
	u.PlayerId = u.DriveGuestId
	return nil
}

func asToIf(in []AccountStatus) []interface{} {
	result := make([]interface{}, len(in))
	for i := 0; i < len(in); i++ {
		result[i] = in[i]
	}
	return result
}

func validateUserAccount(v *validate.Validator, ua *UserAccount) {
	u := ua.User
	v.Field("status", u.Status, validate.OneOf{asToIf(AccountValidStates)})
	switch u.Status {
	case AccountActive:
		earliestDOB := time.Date(1900, time.January, 1, 0, 0, 0, 0, time.UTC).Format("2006-01-02")
		latestDOB := date.Today().Time(0, 0, 0, 0, time.UTC).Format("2006-01-02") // quick learner
		v.Field("dob", u.DOB.String(), validate.MatchAny{                         // DOB may be nil or a valid date
			validate.Equal{Val: ""},
			validate.StringBetween{Min: earliestDOB, Max: latestDOB},
		})
		v.Field("email", u.Email, validate.Email{})
		v.Field("drive_account_id", u.DriveGuestId, validate.CharCountBetween{DriveGuestIdMinLength, DriveGuestIdMaxLength}) // may be nil
		v.Field("family_name", u.FamilyName, validate.CharCountBetween{FamilyNameMinLength, FamilyNameMaxLength})            // may be nil
		v.Field("gender", u.Gender, validate.MatchAny{
			validate.IsNil{},
			validate.OneOfString{ValidGenders},
		})
		v.Field("given_name", u.GivenName, validate.CharCountBetween{GivenNameMinLength, GivenNameMaxLength}) // may be nil
		v.Field("username", u.Username, validate.CharCountBetween{UsernameMinLength, UsernameMaxLength})
		if u.EmailLang != nil { // may be nil if field not specified - Regexp blindly derefs - protect
			v.Field("email_lang", u.EmailLang, validate.MatchRegexp{Regexp: emailLangRegexp})
		}
	case AccountPurged:
		v.Field("email", u.Email, validate.IsNil{})
		v.Field("family_name", u.FamilyName, validate.IsNil{})
		v.Field("given_name", u.GivenName, validate.IsNil{})
		v.Field("username", u.Username, validate.IsNil{})
	}
}

type userWrapper struct {
	User
	BlockedEmail *string `db:"blocked_email"`
}

func usersByClause(clause string, args ...interface{}) ([]*User, error) {
	var results []userWrapper
	_, err := db.Dbmap.Select(&results,
		"SELECT "+db.ColumnsFor["users"]+", blocked_emails.email as blocked_email "+
			"FROM users LEFT JOIN blocked_emails ON (users.email=blocked_emails.email) "+
			"WHERE "+clause, args...)

	if err == sql.ErrNoRows || len(results) == 0 {
		return nil, nil
	}
	if err != nil {
		return nil, err
	}

	users := make([]*User, len(results))
	for i, result := range results {
		results[i].User.PostGet(nil) // ensure hook fires
		users[i] = &results[i].User
		users[i].EmailIsBlocked = result.BlockedEmail != nil
	}
	return users, nil
}

func userByClause(clause string, args ...interface{}) (*User, error) {
	users, err := usersByClause(clause, args...)
	if err != nil {
		return nil, err
	}
	if len(users) > 0 {
		return users[0], nil
	}
	return nil, nil
}

// AccountById fetches a user account from the database by UserId.
//
// If the user is not found, then ua and err will be nil.
func AccountById(userId string) (ua *UserAccount, err error) {
	user, err := userByClause("users.user_id=?", userId)
	if err != nil || user == nil {
		return nil, err
	}

	ua = &UserAccount{User: user}

	return ua, nil
}

// AccountByUsername fetches a user account from the database by active username.
//
// If the user is not found, then ua and err will be nil.
func AccountByUsername(username string) (ua *UserAccount, err error) {
	user, err := userByClause("users.username=?", username)
	if err != nil || user == nil {
		return nil, err
	}

	ua = &UserAccount{User: user}

	return ua, nil
}

// UserByUsername fetches a user from the database
//
// If the user is not found, then ua and err will be nil.
func UserByUsername(username string) (u *User, err error) {
	user, err := userByClause("users.username=?", username)
	if err != nil || user == nil {
		return nil, err
	}
	return user, nil
}

// AccountsByEmail fetches users who have an email address
func AccountsByEmail(email string) (accounts []*UserAccount, err error) {
	users, err := usersByClause("users.email=?", email)
	if err != nil || len(users) == 0 {
		return nil, err
	}

	for _, user := range users {
		ua := &UserAccount{User: user}
		accounts = append(accounts, ua)
	}
	return accounts, nil
}

func trimSpace(fields ...*string) {
	for _, field := range fields {
		if field != nil {
			*field = strings.TrimSpace(*field)
		}
	}
}

// GetBlockedEmail returns a BlockedEmail object for a given email address
// or nil, if the address is not blocked.
func GetBlockedEmail(addr string) (*BlockedEmail, error) {
	obj, err := db.Dbmap.Get(BlockedEmail{}, addr)
	if err != nil {
		alog.Error{"action": "get_blocked_email", "status": "error", "address": addr, "error": err}.Log()
		return nil, err
	}
	if obj == nil {
		return nil, nil
	}
	return obj.(*BlockedEmail), nil
}

// utility to construct (?, ?, ?) SQL markers
func markersForArgs(argCount int) string {
	switch argCount {
	case 0:
		return ""
	case 1:
		return "?"
	default:
		return strings.Repeat("?,", argCount-1) + "?"
	}
}

// CreateUser creates a new user in the database.
//
// This method will validate the supplied data, assign UUIDs and update
// the passed in UserAccount object with dynamic data such as timestamps.
func CreateUserAccount(ua *UserAccount) (err error) {
	v := validate.New()

	// Check the email address isn't blocked
	blocked, err := GetBlockedEmail(autil.NS(ua.User.Email))
	if err != nil {
		return err
	}
	if blocked != nil {
		return apierrors.EmailBlocked
	}

	// trim fields that shouldn't start/end with space characters
	trimSpace(ua.User.Username, ua.User.Email, ua.User.Gender,
		ua.User.FamilyName, ua.User.GivenName, ua.User.DriveGuestId)

	v.Field("password", ua.NewPassword, validate.CharCountBetween{6, 100})
	validateUserAccount(v, ua)
	if verr := v.Validate(); verr != nil {
		return verr
	}
	if ua.NewPassword != nil {
		ua.User.PasswordHash, err = NewHashedPassword(DefaultHasher, []byte(*ua.NewPassword))
		if err != nil {
			return err
		}
		ua.User.PasswordIsComplex, _ = PasswordIsStrong(*ua.User.Username, *ua.NewPassword)

	}
	trans, err := db.Dbmap.Begin()
	if err != nil {
		return err
	}
	if err = trans.Insert(ua.User); err != nil {
		trans.Rollback()
		if db.IsConstraintViolation(err) {
			return apierrors.DuplicateUsername
		}
		return err
	}

	return trans.Commit()
}

func (ua *UserAccount) IsPasswordCorrect(password string) (bool, error) {
	return ua.User.PasswordHash.Validate([]byte(password))
}

// UpdateUserAccount updates an existing user account.
//
// All fields will be replaced.
func UpdateUserAccount(ua *UserAccount) (err error) {
	v := validate.New()
	if ua.NewPassword != nil {
		v.Field("password", ua.NewPassword, validate.CharCountBetween{6, 100})
	}

	// trim fields that shouldn't start/end with space characters
	trimSpace(ua.User.Username, ua.User.Email, ua.User.Gender,
		ua.User.FamilyName, ua.User.GivenName, ua.User.DriveGuestId)

	validateUserAccount(v, ua)
	if verr := v.Validate(); verr != nil {
		return verr
	}

	if ua.NewPassword != nil {
		// apply updated password, if supplied
		ua.User.PasswordHash, err = NewHashedPassword(DefaultHasher, []byte(*ua.NewPassword))
		if err != nil {
			return err
		}
		ua.User.PasswordIsComplex, _ = PasswordIsStrong(*ua.User.Username, *ua.NewPassword)
		now := time.Now()
		ua.User.TimePasswordUpdated = &now
	}

	trans, err := db.Dbmap.Begin()
	if err != nil {
		return err
	}
	if _, err = trans.Update(ua.User); err != nil {
		trans.Rollback()
		if db.IsConstraintViolation(err) {
			return apierrors.DuplicateUsername
		}
		return err
	}

	return trans.Commit()
}

// DeactivateUserAccount updates the status of a user, but does not actaully delete it.
func DeactivateUserAccount(userId, reason string) error {
	if reason == "" {
		return errors.New("No deactivation reason provided")
	}

	ua, err := AccountById(userId)
	if err != nil {
		return err
	}
	if ua == nil {
		return apierrors.AccountNotFound
	}

	switch ua.User.Status {
	case AccountActive:
	case AccountDeactivated:
		return nil // no-op
	case AccountPurged:
		return nil // no-op
	default:
		return errors.New("Account in unknown state")
	}

	ua.User.Status = AccountDeactivated
	now := time.Now()
	ua.User.TimeDeactivated = &now
	ua.User.DeactivatedUsername = ua.User.Username
	ua.User.DeactivationReason = &reason
	ua.User.Username = nil

	return UpdateUserAccount(ua)
}

// PurgeUserAccount removes all PII info from a user account.  It is not reversible.
func PurgeUserAccount(userId, reason string) error {
	if reason == "" {
		return errors.New("No deactivation reason provided")
	}

	ua, err := AccountById(userId)
	if err != nil {
		return err
	}
	if ua == nil {
		return apierrors.AccountNotFound
	}

	switch ua.User.Status {
	case AccountDeactivated:
	case AccountActive:
		return apierrors.CannotPurgeActive
	case AccountPurged:
		return apierrors.AccountAlreadyPurged
	default:
		return errors.New("Account in unknown state")
	}

	ua.User.Status = AccountPurged
	now := time.Now()
	ua.User.TimePurged = &now
	ua.User.PurgeReason = &reason
	ua.User.Username = nil
	ua.User.Email = nil
	ua.User.DOB = date.Zero
	ua.User.GivenName = nil
	ua.User.FamilyName = nil

	return UpdateUserAccount(ua)
}

// ReactivateUserAccount attempts to reactivate a user using the supplied username
// Will return UserNotFound if the user doesn't exist or a duplicate username
// error if the username is already in use.
func ReactivateUserAccount(userId, username string) error {
	ua, err := AccountById(userId)
	if err != nil {
		return err
	}
	if ua == nil {
		return apierrors.AccountNotFound
	}

	switch ua.User.Status {
	case AccountDeactivated:
	case AccountActive:
		return apierrors.AccountAlreadyActive
	case AccountPurged:
		return apierrors.CannotReactivatePurged
	default:
		return errors.New("Account in unknown state")
	}

	ua.User.Status = AccountActive
	ua.User.Username = &username

	// reset the account activation state machine; if the account wasn't activated
	// before, it should restart the 15 day period to activaate it.
	ua.User.DeactivationNoticeState = NoDeactivationNoticeSent
	ua.User.TimeDeactivationStateUpdated = nil

	return UpdateUserAccount(ua)
}

var (
	// Create a task runner for the update inactive task.  This is also hooked into the users api
	UpdateUnverifiedRunner = task.NewRunner(task.TaskFunc(UpdateUnverified))
)

type userid struct {
	Id string `db:"user_id"`
}

// UpdateUnverified deactivtes user accounts that have not been confirmed within
// the configured timeframe
func UpdateUnverified(frm url.Values) error {
	// This routine performs bulk update operations in a transaction, rather than loading
	// the full user account of all affected users into memory.
	var (
		earliest time.Time
		err      error
		start    = time.Now()
	)

	if maxIdleStr := frm.Get("max-idle"); maxIdleStr != "" {
		maxIdle, err := time.ParseDuration(maxIdleStr)
		if err != nil {
			alog.Error{"action": "update_inactive_task", "status": "max_idle_parse_failure", "error": err}.Log()
			return fmt.Errorf("Failed to parse max-idle parameter: %s", err)
		}
		earliest = time.Now().Add(-maxIdle)
	} else {
		earliest = time.Now().Add(-deactivationPeriodThree)
	}

	alog.Info{"action": "update_inactive_task", "status": "starting", "since": earliest}.Log()
	trans, err := db.Dbmap.Begin()
	if err != nil {
		alog.Error{"action": "update_inactive_task", "status": "txn_start_failed", "error": err}.Log()
		return err
	}

	// Fetch user ids of all users that have been sent the second deactivation warning
	// Max of 500 per call
	userids, err := trans.Select(
		userid{},
		"SELECT user_id FROM users "+
			"WHERE status=? "+
			"AND time_email_first_verified IS NULL "+
			"AND deactivation_notice_state=? "+
			"AND time_deactivation_state_updated < ? "+
			"AND no_autodelete=0 "+
			"LIMIT "+strconv.Itoa(maxUsersPerDeactivate)+
			" "+db.ForUpdate,
		AccountActive, SecondDeactivationNoticeSent, earliest)
	if err != nil {
		alog.Error{"action": "update_inactive_task", "status": "select_failed", "error": err}.Log()
		trans.Rollback()
		return err
	}

	if len(userids) == 0 {
		// nothing to do
		runtime := time.Now().Sub(start)
		alog.Info{"action": "update_inactive_task", "status": "success", "users_affected": 0, "runtime_seconds": runtime.Seconds()}.Log()
		trans.Rollback()
		return nil
	}

	uids := make([]string, len(userids))
	for i, entry := range userids {
		uids[i] = `"` + entry.(*userid).Id + `"`
	}
	userList := strings.Join(uids, ",")

	// Update status of users
	if _, err := trans.Exec(
		"UPDATE users SET status=?, "+
			"deactivated_username=username, "+
			"username=NULL, "+
			"time_deactivated=?, "+
			"deactivation_reason=? "+
			"WHERE user_id IN ("+userList+")", AccountDeactivated, time.Now(), deactivationTimeoutReason); err != nil {
		alog.Error{"action": "update_inactive_task", "status": "update_failed", "error": err}.Log()
		trans.Rollback()
		return err
	}

	if err := trans.Commit(); err != nil {
		alog.Error{"action": "update_inactive_task", "status": "commit_failed", "error": err}.Log()
		trans.Rollback()
		return err
	}

	for _, entry := range userids {
		alog.Info{"action": "update_inactive_task", "status": "deactivated_user", "userid": entry.(*userid).Id}.Log()
	}

	// capture timing info
	runtime := time.Now().Sub(start)
	alog.Info{"action": "update_inactive_task", "status": "success", "users_affected": len(uids), "runtime_seconds": runtime.Seconds()}.Log()

	return nil
}

var (
	// Create a task runner for the purge task.  This is also hooked into the users api
	PurgeDeactivatedRunner = task.NewRunner(task.TaskFunc(PurgeDeactivated))
)

// PurgeDeactivated clears all PII from user accounts that have been deactivated
// for longer than the configured timeframe
func PurgeDeactivated(frm url.Values) error {
	var earliest time.Time

	start := time.Now()
	if maxIdleStr := frm.Get("max-idle"); maxIdleStr != "" {
		maxIdle, err := time.ParseDuration(maxIdleStr)
		if err != nil {
			alog.Error{"action": "purge_users_task", "status": "max_idle_parse_failure", "error": err}.Log()
			return fmt.Errorf("Failed to parse max-idle parameter: %s", err)
		}
		earliest = time.Now().Add(-maxIdle)
	} else {
		earliest = time.Now().Add(-deactivatePurgeTimeout)
	}

	alog.Info{"action": "update_inactive_task", "status": "starting", "since": earliest}.Log()

	trans, err := db.Dbmap.Begin()
	if err != nil {
		alog.Error{"action": "purge_users_task", "status": "txn_start_failed", "error": err}.Log()
		return err
	}

	type userid struct {
		Id string `db:"user_id"`
	}

	// Fetch user ids of all users that have been sent the second deactivation warning
	// Max of 500 per call.
	userids, err := trans.Select(
		userid{},
		"SELECT user_id FROM users "+
			"WHERE status=? AND time_deactivated < ? AND no_autodelete=0 "+
			"LIMIT "+strconv.Itoa(maxUsersPerPurge)+
			" "+db.ForUpdate,
		AccountDeactivated, earliest)
	if err != nil {
		alog.Error{"action": "purge_users_task", "status": "select_failed", "error": err}.Log()
		trans.Rollback()
		return err
	}

	if len(userids) == 0 {
		// nothing to do
		runtime := time.Now().Sub(start)
		alog.Info{"action": "purge_users_task", "status": "success", "users_affected": 0, "runtime_seconds": runtime.Seconds()}.Log()
		trans.Rollback()
		return nil
	}

	uids := make([]string, len(userids))
	for i, entry := range userids {
		uids[i] = `"` + entry.(*userid).Id + `"`
	}
	userList := strings.Join(uids, ",")

	result, err := trans.Exec(`UPDATE users SET `+
		`status=?, `+
		`dob="", `+
		`email=NULL, `+
		`family_name=NULL, `+
		`gender=NULL, `+
		`given_name=NULL, `+
		`time_purged=?, `+
		`purge_reason=? `+
		`WHERE user_id IN (`+userList+`)`,
		AccountPurged, time.Now(), purgeTimeoutReason)
	if err != nil {
		alog.Error{"action": "purge_users_task", "status": "update_failed", "error": err}.Log()
		trans.Rollback()
		return err
	}

	if err := trans.Commit(); err != nil {
		alog.Error{"action": "purge_users_task", "status": "commit_failed", "error": err}.Log()
		trans.Rollback()
		return err
	}

	// Log affected users
	for _, entry := range userids {
		alog.Info{"action": "purge_users_task", "status": "purged_user", "userid": entry.(*userid).Id}.Log()
	}

	// capture timing info
	runtime := time.Now().Sub(start)
	count, _ := result.RowsAffected()
	alog.Info{"action": "purge_users_task", "status": "success", "users_affected": count, "runtime_seconds": runtime.Seconds()}.Log()

	return nil
}

// Related returns active accounts that share the same email address
func (u *User) Related() ([]*User, error) {
	return usersByClause("users.email=? AND users.status=? AND users.user_id!=?",
		u.Email, AccountActive, u.Id)
}

func (u *User) SetEmailFailureCode(code string) error {
	_, err := db.Dbmap.Exec("UPDATE users SET email_failure_code=? WHERE user_id=?", code, u.Id)
	return err
}

type emailSender func() error

func (u *User) sendEmail(emailName string, msg *email.AnkiEmail) error {
	addr := autil.NS(u.Email)
	if failCode := autil.NS(u.EmailFailureCode); failCode != "" {
		alog.Warn{"action": "send_email_to_user", "email_name": emailName, "status": "email_address_previously_failed",
			"failure_code": failCode, "address": addr, "userid": u.Id}.Log()
		return InvalidEmail
	}

	if u.EmailIsBlocked {
		alog.Warn{"action": "send_email_to_user", "email_name": emailName, "status": "email_address_is_blocked",
			"address": addr, "userid": u.Id}.Log()
		return InvalidEmail
	}

	if err := email.Emailer.Send(msg); err != nil {
		u.handleEmailFailure(nil, addr, emailName, err)
		return err
	}
	alog.Info{"action": "send_email_to_user", "email_name": emailName, "status": "sent_ok", "userid": u.Id, "address": addr}.Log()
	return nil
}

func (u *User) handleEmailFailure(txn *gorp.Transaction, addr, emailName string, emailErr error) error {
	var err error
	if txn == nil {
		txn, err = db.Dbmap.Begin()
		if err != nil {
			return err
		}
		defer txn.Commit()
	}
	if aerr, ok := emailErr.(*email.SendError); ok {
		switch aerr.Code() {
		case email.InvalidEmail, email.InactiveRecipient:
			alog.Warn{"action": "send_email_to_user", "email_name": emailName,
				"status": "email_send_invalid_address", "userid": u.Id, "address": addr,
				"code": aerr.Code(), "error": emailErr}.Log()
			// mark address as bad for all accounts sharing the same email address.
			if err := markAddressAsFailed(txn, addr, "invalid_address"); err != nil {
				alog.Error{"action": "send_email_to_user", "address": addr, "status": "mark_failed_db_error",
					"userid": u.Id, "error": err}.Log()
				return err
			}
		default:
			alog.Error{"action": "send_email_to_user", "email_name": emailName, "status": "email_send_failed",
				"userid": u.Id, "address": addr, "code": aerr.Code(), "error": emailErr}.Log()
		}
	} else {
		alog.Error{"action": "send_email_to_user", "email_name": emailName, "status": "email_send_failed",
			"userid": u.Id, "address": addr, "error": emailErr}.Log()
	}
	return nil
}

// Set the email failure code for all accounts with the same email address
func markAddressAsFailed(txn *gorp.Transaction, email, code string) error {
	_, err := txn.Exec("UPDATE users SET email_failure_code=? WHERE email=?", code, email)
	return err
}

func MarkAddressAsFailed(email, code string) error {
	_, err := db.Dbmap.Exec("UPDATE users SET email_failure_code=? WHERE email=?", code, email)
	return err
}

func (ua *UserAccount) SendVerificationEmail() error {
	u := ua.User
	isChild, err := ua.IsChild()
	if err != nil {
		alog.Error{"action": "create_user", "status": "dob_error", "user_id": u.Id, "error": err}.Log()
		return err
	}

	msg := email.VerificationEmail(
		email.Emailer,
		autil.NS(ua.User.GivenName),
		autil.NS(ua.User.FamilyName),
		autil.NS(ua.User.Email), email.AccountInfo{
			Username:         autil.NS(u.Username),
			UserId:           u.Id,
			Adult:            !isChild,
			DOB:              u.DOB.String(),
			Token:            autil.NS(u.EmailVerify),
			EmailLang:        u.EmailLang,
			CreatedByAppName: autil.NS(u.CreatedByAppName),
		}, nil)
	return u.sendEmail("email_verification", msg)
}

func SendForgotPasswordByUserId(userId string) ([]*UserAccount, error) {
	ua, err := AccountById(userId)
	if err != nil {
		return nil, err
	}
	if ua == nil || ua.Status != AccountActive {
		return nil, apierrors.AccountNotFound
	}
	return []*UserAccount{ua}, sendForgotPassword([]*UserAccount{ua})
}

func SendForgotPasswordByUsername(username string) ([]*UserAccount, error) {
	ua, err := AccountByUsername(username)
	if err != nil {
		return nil, err
	}
	if ua == nil || ua.Status != AccountActive {
		return nil, apierrors.AccountNotFound
	}
	return []*UserAccount{ua}, sendForgotPassword([]*UserAccount{ua})
}

func SendForgotPasswordByEmail(email string) ([]*UserAccount, error) {
	uas, err := AccountsByEmail(email)
	if err != nil {
		return nil, err
	}
	var filtered []*UserAccount
	for _, ua := range uas {
		if ua.Status == AccountActive {
			filtered = append(filtered, ua)
		}
	}
	if len(filtered) == 0 {
		return nil, apierrors.AccountNotFound
	}
	return filtered, sendForgotPassword(filtered)
}

func sendForgotPassword(users []*UserAccount) error {
	var (
		accinfos []email.AccountInfo
		userIds  []string
	)
	toEmail := *users[0].User.Email
	for i, u := range users {
		if !strings.EqualFold(*u.User.Email, toEmail) {
			// should never happen!
			return fmt.Errorf("sendForgotPassword called with users with differing email addreses.  "+
				"toEmail=%q currentEmail=%q uas[0]=%q uas[%d]=%q",
				toEmail, *u.User.Email, users[0].User.Id, i, u.User.Id)
		}
		if u.User.Status == AccountActive {
			tt := timedtoken.Token()
			u.SetPasswordResetToken(tt)
			accinfos = append(accinfos,
				email.AccountInfo{
					Username:         *u.User.Username,
					UserId:           u.User.Id,
					Adult:            true,
					Token:            *u.User.PasswordReset,
					EmailLang:        u.EmailLang,
					CreatedByAppName: autil.NS(u.CreatedByAppName),
				})
			userIds = append(userIds, u.User.Id)

		} else {
			// should not happen as the caller should have filtered already
			alog.Error{"action": "resetuserpassword", "userid": u.User.Id, "status": "tried_to_reset_inactive_user_password"}.Log()
		}
	}

	alog.Info{"action": "resetuserpassword", "status": "pwreset_email", "num_total_accounts": len(users),
		"num_accounts_active": len(accinfos), "userids": strings.Join(userIds, ",")}.Log()
	msg := email.ForgotPasswordEmail(email.Emailer, "", "", toEmail, accinfos, nil)
	return users[0].sendEmail("resetuserpassword", msg)
}

func (ua *UserAccount) SendPostVerificationEmail() error {
	// Note emails are only sent to users < 13
	u := ua.User
	isChild, err := ua.IsChild()
	if err != nil {
		alog.Error{"action": "send_post_verification_email", "status": "dob_error", "user_id": u.Id, "error": err}.Log()
		return err
	}
	if !isChild {
		// No action needs to be taken - we can short circuit
		return nil
	}

	msg := email.PostVerificationEmail(
		email.Emailer,
		autil.NS(ua.User.GivenName),
		autil.NS(ua.User.FamilyName),
		autil.NS(ua.User.Email), email.AccountInfo{
			Username:         autil.NS(u.Username),
			UserId:           u.Id,
			EmailLang:        u.EmailLang,
			CreatedByAppName: autil.NS(u.CreatedByAppName),
		})
	return u.sendEmail("email_account_parental_post_verify", msg)
}

func (ua *UserAccount) SendAccountUpdatedEmail() error {
	u := ua.User
	msg := email.AccountUpdatedEmail(
		email.Emailer,
		autil.NS(ua.User.GivenName),
		autil.NS(ua.User.FamilyName),
		autil.NS(ua.User.Email), email.AccountInfo{
			Username:         autil.NS(u.Username),
			UserId:           u.Id,
			EmailLang:        u.EmailLang,
			CreatedByAppName: autil.NS(u.CreatedByAppName),
		})
	return u.sendEmail("email_verification_account_updated", msg)
}

// Send a notification email to account holders who have only 7 days left to validate
// their email address.
func notifyUnverified(frm url.Values, noticeState DeactivationNoticeState) error {
	var (
		timeSince  time.Duration
		timeClause string
		sendEmail  email.VerificationEmailFunc
		newState   DeactivationNoticeState
		start      = time.Now()
		mode       string
	)

	switch noticeState {
	case NoDeactivationNoticeSent:
		// notify 7 days before the 15 day deactivation timeout
		timeClause = "AND time_created < ?"
		timeSince = userConfirmationTimeout - deactivateNotifyPeriodOne
		sendEmail = email.Verification7DayReminderEmail
		newState = FirstDeactivationNoticeSent
		mode = "7-day"

	case FirstDeactivationNoticeSent:
		// notify 6 days after the previous email was sent (1 day till deactivation)
		timeClause = "AND time_deactivation_state_updated < ?"
		timeSince = deactivateNotifyPeriodTwo
		sendEmail = email.Verification14DayReminderEmail
		newState = SecondDeactivationNoticeSent
		mode = "14-day"

	default:
		panic(fmt.Sprintf("notifyUnverified called with unexpected noticeState=%v", noticeState))
	}

	if notifyDuration := frm.Get("notify-older-than"); notifyDuration != "" {
		d, err := time.ParseDuration(notifyDuration)
		if err != nil {
			alog.Error{"action": "notify_unverified_task", "mode": mode, "status": "notify_older_than_parse_failure", "error": err}.Log()
			return fmt.Errorf("Failed to parse max-idle parameter: %s", err)
		}
		timeSince = d
	}

	// process a maximum of 250 users per run to avoid saturating postmark.
	txn, err := db.Dbmap.Begin()
	if err != nil {
		alog.Error{"action": "notify_unverified_task", "mode": mode, "status": "db_begin_failed", "error": err}.Log()
		return errors.New("Failed to start transaction")
	}

	type notifyResult struct {
		User                // Gorp will populate anonymous structs as if the fields were inline
		BlockedAddr *string `db:"blocked_email"`
	}

	users, err := txn.Select(
		notifyResult{},
		"SELECT "+db.ColumnsFor["users"]+", blocked_emails.email as blocked_email FROM users "+
			"LEFT JOIN blocked_emails ON (users.email=blocked_emails.email) "+
			"WHERE status=? "+
			"AND time_email_first_verified IS NULL "+
			"AND email_is_verified=0 "+
			"AND deactivation_notice_state=? "+
			timeClause+" LIMIT "+strconv.Itoa(maxEmailsPerBatch),
		AccountActive,
		noticeState,
		time.Now().Add(-timeSince))

	if err != nil {
		alog.Error{"action": "notify_unverified_task", "mode": mode, "status": "failed", "error": err}.Log()
		txn.Rollback()
		return err
	}

	if len(users) == 0 {
		runtime := time.Now().Sub(start)
		alog.Info{"action": "notify_unverified_task", "mode": mode, "status": "ok", "users_affected": len(users), "runtime_seconds": runtime.Seconds()}.Log()
		txn.Rollback()
		return nil
	}

	msgs := make([]*email.AnkiEmail, 0)

	emailName := "notify_unverified" + strconv.Itoa(int(noticeState))
	for i, row := range users {
		result := row.(*notifyResult)
		u := &result.User
		ua := &UserAccount{User: u}
		isChild, _ := ua.IsChild()
		if u.Username == nil {
			alog.Error{"action": "notify_unverified_task", "mode": mode, "status": "active_user_without_username", "userid": u.Id}.Log()
			continue
		}
		if u.EmailVerify == nil {
			alog.Error{"action": "notify_unverified_task", "mode": mode, "status": "unverified_user_without_token", "userid": u.Id}.Log()
			continue
		}

		hasValidEmail := autil.NS(u.EmailFailureCode) == ""
		isBlocked := result.BlockedAddr != nil

		ui := email.AccountInfo{
			UserId:           u.Id,
			Username:         *u.Username,
			Adult:            !isChild,
			DOB:              u.DOB.String(),
			Token:            *u.EmailVerify,
			EmailLang:        u.EmailLang,
			CreatedByAppName: autil.NS(u.CreatedByAppName),
			TimeCreated:      u.TimeCreated,
		}

		// This deliberately does not defer email sending to a goroutine so we don't flood the remote server
		// one day we might need a better way of doing this
		if !hasValidEmail {
			// If the user's email address has previously been rejected by postmark, we don't
			// want to try sending to it again.  We do want to move the state machine along, however
			alog.Info{"action": "notify_unverified_task", "status": "skipped_email_send", "mode": mode,
				"userid": u.Id, "address": *u.Email, "reason": "email_address_marked_invalid"}.Log()

		} else if isBlocked {
			alog.Info{"action": "notify_unverified_task", "status": "skipped_email_send", "mode": mode,
				"userid": u.Id, "address": *u.Email, "reason": "email_address_blocked"}.Log()

		} else {
			msg := sendEmail(email.Emailer, autil.NS(u.GivenName), autil.NS(u.FamilyName), *u.Email, ui, nil)
			msg.Metadata = u
			msgs = append(msgs, msg)
		}

		if _, err := txn.Exec(
			"UPDATE users SET deactivation_notice_state=?, time_deactivation_state_updated=?  WHERE user_id=?",
			newState, time.Now(), u.Id); err != nil {
			alog.Error{"action": "notify_unverified_task", "mode": mode, "status": "db_update_failed", "userid": u.Id, "error": err}.Log()
			// abort the whole thing; will retry another time.
			alog.Error{"action": "notify_unverified_task", "mode": mode, "status": "aborted", "completed": i, "remaining": len(users) - i}.Log()
			txn.Rollback()
			return err
		}
	}

	// Attempt to send the messages in bulk
	if len(msgs) > 0 {
		results, err := email.Emailer.SendBatch(msgs)
		if err != nil {
			// hard fail
			alog.Error{"action": "notify_unverified_task", "mode": mode, "status": "batch_email_failed", "error": err}.Log()
			txn.Rollback()
			return err
		}

		for _, result := range results {
			u := result.Email.Metadata.(*User)
			if result.Error != nil {
				u.handleEmailFailure(txn, result.Email.ToEmail, emailName, result.Error)
			} else {
				alog.Info{"action": "notify_unverified_task", "mode": mode, "email_name": emailName,
					"status": "sent_ok", "userid": u.Id, "address": result.Email.ToEmail}.Log()
			}
		}
	}

	if err := txn.Commit(); err != nil {
		alog.Info{"action": "notify_unverified_task", "mode": mode, "status": "db_commit_failed", "error": err}.Log()
		return nil
	}

	// capture timing info
	runtime := time.Now().Sub(start)
	alog.Info{"action": "notify_unverified_task", "mode": mode, "status": "ok", "users_affected": len(users), "runtime_seconds": runtime.Seconds()}.Log()
	return nil
}

var (
	// Create a task runner for the update inactive task.  This is also hooked into the users api
	NotifyUnverifiedRunner = task.NewRunner(task.TaskFunc(NotifyUnverified))
)

// Send a notification email to account holders who have only 7 days left to validate
// their email address.
func NotifyUnverified(frm url.Values) error {
	// warn all those that are within 24 hours of deactivation
	if err := notifyUnverified(frm, FirstDeactivationNoticeSent); err != nil {

		// notify all those hitting the 7 day notice level
		return err
	}
	return notifyUnverified(frm, NoDeactivationNoticeSent)
}

// SetEmailValidationToken clears the user's EmailIsVerified flag and
// associated a verification token with the account.
func (ua *UserAccount) SetEmailValidationToken(token string) error {
	if token == "" {
		return errors.New("No token provieded to SetEmailValidationToken")
	}
	ua.User.EmailIsVerified = false
	ua.User.EmailVerify = &token
	return UpdateUserAccount(ua)
}

// SetEmailValidated marks the user as having a valid email address and removes
// any validation token that was associated with the account
func (ua *UserAccount) SetEmailValidated() error {
	ua.User.EmailIsVerified = true
	ua.User.EmailVerify = nil
	if ua.User.TimeEmailFirstVerified == nil {
		now := time.Now()
		ua.User.TimeEmailFirstVerified = &now
	}
	return UpdateUserAccount(ua)
}

// PasswordRestToken
func (ua *UserAccount) SetPasswordResetToken(token string) error {
	ua.User.PasswordReset = &token
	return UpdateUserAccount(ua)
}

// PasswordUpdateByToken performs the steps needed to update the user when they correctly supplied
// a password reset token.
func (ua *UserAccount) PasswordUpdateByToken(token, password string) error {
	if !ua.IsValidPasswordToken(token) {
		return InvalidPasswordToken
	}
	ua.User.PasswordReset = nil
	ua.NewPassword = &password
	return UpdateUserAccount(ua)
}

// GenerateSuggestions creates some alternative username suggestions based on the
// supplied username.
//
// Suggestions are checked for availability prior to being returned.
//
// This is a very simple algorithm:
// 1) Strip whitespace from the beginning/end of the requested username
// 2) Strip any unicode digits off the end of the username until its length is equal to the
// minimum username length (currently 3 characters)
// 3) Add a number to the end of the username and increment it for each suggestion
// 4) If the suggested username is now too long, strip a character off of the original username before
// addding the number to it
// 5) Keep trying suggestions until we have enough non-taken usernames to return to the user.
//
// This routine is unicode/utf8 aware.
func GenerateSuggestions(basename string, count int) (result []string, err error) {
	trimSpace(&basename)
	v := validate.New()
	v.Field("username", basename, validate.CharCountBetween{UsernameMinLength, UsernameMaxLength})
	if verr := v.Validate(); verr != nil {
		return nil, verr
	}

	suggestions := make([]string, 1, count)
	// ensure we check the original username too, in case it's actually valid
	suggestions[0] = basename

	// strip digits off the end of the username until we hit the min length requirement
	baselength := utf8.RuneCountInString(basename)
	for baselength > UsernameMinLength {
		r, size := utf8.DecodeLastRuneInString(basename)
		if r == utf8.RuneError {
			return nil, apierrors.InvalidUTF8
		}
		if !unicode.IsDigit(r) {
			break
		}
		basename = basename[:len(basename)-size]
		baselength--
	}

	attempt := 0
	for len(result) < count {
		for i := len(result); i < count; i++ {
			num := strconv.Itoa(attempt + 1)
			if baselength+len(num) > UsernameMaxLength {
				// strip last rune to make space for a digit
				_, runelen := utf8.DecodeLastRuneInString(basename)
				basename = basename[:len(basename)-runelen]
				baselength--
			}
			suggestions = append(suggestions, basename+num)
			attempt++
		}

		// filter out usernames that are already in use
		usernames, err := filterUsernames(suggestions)
		if err != nil {
			return nil, err
		}
		result = append(result, usernames...)
		suggestions = suggestions[0:0] // truncate
	}

	// if the original username was valid, result will contain one more suggestion than requested.
	return result[:count], nil
}

func filterUsernames(usernames []string) ([]string, error) {
	// Execute a single DB query to see which names are not already in use
	// This does a left join against a union table so the name comparisons are
	// handled directly by MySQL so we don't have to recreate its unicode collation rules
	// in Go.

	if len(usernames) == 0 {
		return usernames, nil
	}

	clauses := make([]string, len(usernames))
	for i := 0; i < len(usernames); i++ {
		// add resultorder so we get the original suggestion first and integer order after
		clauses[i] = fmt.Sprintf(" SELECT ? AS checkuser, %d AS resultorder ", i)
	}

	// MySQL is broken for parametized subselects with only one select http://bugs.mysql.com/bug.php?id=71577
	// workaround with a dedicated query isntead
	if len(usernames) == 1 {
		count, err := db.Dbmap.SelectInt("SELECT COUNT(*) FROM users WHERE username=?", usernames[0])
		if err != nil {
			return nil, err
		}
		if count == 0 {
			return usernames, nil
		}
		return []string{}, nil
	}

	// query to identify which of the specified usernames are NOT already in use
	query := fmt.Sprintf(
		"SELECT intab.checkuser FROM "+
			"(%s) AS intab LEFT JOIN users "+
			"ON intab.checkuser=users.username WHERE users.username IS NULL ORDER BY intab.resultorder",
		strings.Join(clauses, " UNION "))

	var unused []string
	args := make([]interface{}, len(usernames))
	for i, username := range usernames {
		args[i] = username
	}
	_, err := db.Dbmap.Select(&unused, query, args...)
	if err != nil {
		return nil, err
	}

	return unused, err
}

var (
	ValidModes   = []string{PrefixFilter, MatchFilter}
	ValidFilters = []string{UserId, Email, Username, FamilyName}
	ValidOrders  = []string{Username, FamilyName, Email}
	ValidStatus  = []interface{}{AccountActive, AccountDeactivated, AccountPurged}
)

func validateListAccounts(v *validate.Validator, modifier AccountListModifier) {
	for _, status := range modifier.Status {
		v.Field("account status", status, validate.OneOf{ValidStatus})
	}
	for _, filter := range modifier.Filters {
		v.Field(filter.Name, filter.Name, validate.OneOfString{ValidFilters})
		v.Field(filter.Name+"_mode", filter.Mode, validate.OneOfString{ValidModes})
	}
	v.Field("limit", modifier.Limit, validate.IntBetween{Min: 1, Max: int64(MaxListLimit)})
	v.Field("order", modifier.OrderBy, validate.OneOfString{ValidOrders})
	v.Field("offset", modifier.Offset, validate.IntBetween{Min: 0, Max: math.MaxInt64})
}

// ListAccounts fetches accounts from the database
func ListAccounts(modifier AccountListModifier) (accounts []*UserAccount, err error) {
	var users []*User
	var clauses []string
	var args []interface{}

	v := validate.New()
	validateListAccounts(v, modifier)
	if verr := v.Validate(); verr != nil {
		return nil, verr
	}

	var statusClause []string
	for i := 0; i < len(modifier.Status); i++ {
		statusClause = append(statusClause, "status=?")
	}
	if len(statusClause) > 1 {
		clauses = append(clauses, "("+strings.Join(statusClause, " OR ")+")")
	} else {
		clauses = append(clauses, statusClause[0])
	}
	searchInactive := false
	for _, status := range modifier.Status {
		args = append(args, string(status))
		if status == AccountDeactivated || status == AccountPurged {
			searchInactive = true
		}
	}

	for _, filter := range modifier.Filters {
		switch filter.Mode {
		case PrefixFilter:
			if filter.Name == Username && searchInactive { // Special case for username when searching deactivated and purged accounts
				clauses = append(clauses, " AND (username LIKE ? OR deactivated_username LIKE ?)")
				args = append(args, filter.Value+"%", filter.Value+"%")
			} else {
				clauses = append(clauses, fmt.Sprintf(" AND %s LIKE ?", filter.Name))
				args = append(args, filter.Value+"%")
			}
		case MatchFilter:
			if filter.Name == Username && searchInactive { // Special case for username when searching deactivated and purged accounts
				clauses = append(clauses, " AND (username=? OR deactivated_username=?)")
				args = append(args, filter.Value, filter.Value)
			} else {
				clauses = append(clauses, fmt.Sprintf(" AND %s=?", filter.Name))
				args = append(args, filter.Value)
			}
		}
	}

	var orderClause string
	if modifier.SortDesc {
		orderClause = modifier.OrderBy + " DESC" + ",user_id"
	} else {
		orderClause = modifier.OrderBy + " ASC" + ",user_id"
	}

	query := fmt.Sprintf("SELECT "+db.ColumnsFor["users"]+" FROM users WHERE %s ORDER BY %s LIMIT %d OFFSET %d",
		strings.Join(clauses, ""),
		orderClause,
		modifier.Limit,
		modifier.Offset)
	_, err = db.Dbmap.Select(&users, query, args...)
	if err == sql.ErrNoRows {
		return nil, nil
	}
	if err != nil {
		return nil, err
	}

	for _, user := range users {
		ua := &UserAccount{User: user}
		accounts = append(accounts, ua)
	}
	return accounts, err
}

// WriteCSV dumps some fields from the users table to the supplied io.Writer in CSV format
// optionally filtering to a time-created window.
// earliestCreateTime and latestCreateTime should be in time.RFC3339 format, or empty.
func WriteCSV(w io.Writer, earliestCreateTime, latestCreateTime string, maxResults int) (count int, err error) {
	query := `SELECT user_id, username, email, dob, created_by_app_name FROM users `
	var clauses []string
	var args []interface{}
	if earliestCreateTime != "" {
		clauses = append(clauses, "time_created >= ?")
		t, err := time.Parse(time.RFC3339, earliestCreateTime)
		if err != nil {
			return 0, errors.New("Invalid earliestCreateTime")
		}
		args = append(args, t)
	}
	if latestCreateTime != "" {
		clauses = append(clauses, "time_created <= ?")
		t, err := time.Parse(time.RFC3339, latestCreateTime)
		if err != nil {
			return 0, errors.New("Invalid latestCreateTime")
		}
		args = append(args, t)
	}
	query += "WHERE status='A'"
	if len(clauses) > 0 {
		query += "AND " + strings.Join(clauses, " AND ")
	}
	query += " ORDER BY time_created"
	if maxResults > 0 {
		query += fmt.Sprintf(" LIMIT %d", maxResults)
	}

	rows, err := db.Dbmap.Db.Query(query, args...)
	if err != nil {
		return 0, err
	}

	cw := csv.NewWriter(w)
	defer rows.Close()
	var dob date.Date
	for rows.Next() {
		var (
			userId, userName, email, dobstr, createdByApp *string
		)

		if err = rows.Scan(&userId, &userName, &email, &dobstr, &createdByApp); err != nil {
			return 0, err
		}

		err := dob.UnmarshalText([]byte(autil.NS(dobstr)))
		if err != nil {
			alog.Error{"action": "writecsv", "status": "invalid_dob_field", "userid": userId}.Log()
			continue
		}

		cw.Write([]string{autil.NS(userId), autil.NS(userName), autil.NS(email), dob.String(), autil.NS(createdByApp)})
		count++
	}

	cw.Flush()

	return count, nil
}

const (
	emailMaxResults = 200000
)

var (
	// Create a task runner for the email CSV task
	EmailCSVRunner = task.NewRunner(task.TaskFunc(EmailCSV))
)

// EmailCSV is triggered as a task that exports a list of users and attaches to an email
func EmailCSV(frm url.Values) error {
	var b bytes.Buffer
	targetEmail := frm.Get("email")
	earliestCreateTime := frm.Get("earliest-create")
	latestCreateTime := frm.Get("latest-create")

	start := time.Now()
	alog.Info{"action": "email_csv", "status": "starting"}.Log()

	if targetEmail == "" {
		alog.Error{"action": "email_csv", "status": "no_email_supplied"}.Log()
		return errors.New("No email field supplied")
	}

	if !strings.HasSuffix(targetEmail, "@anki.com") {
		alog.Error{"action": "email_csv", "status": "non-anki email address", "email": targetEmail}.Log()
		return errors.New("Non anki.com email field supplied")
	}

	zipper := gzip.NewWriter(&b)
	count, err := WriteCSV(zipper, earliestCreateTime, latestCreateTime, emailMaxResults)
	// capture timing info
	runtime := time.Now().Sub(start)

	if err != nil {
		alog.Error{"action": "email_csv", "status": "csv_gen_failed", "runtime_seconds": runtime.Seconds(), "error": err}.Log()
		return err
	}

	if count == 0 {
		// don't send an email if no matches
		alog.Info{"action": "email_csv", "status": "completed", "count": count, "runtime_seconds": runtime.Seconds()}.Log()
		return nil
	}

	if err := zipper.Close(); err != nil {
		alog.Error{"action": "email_csv", "status": "gzip_failed", "count": count, "runtime_seconds": runtime.Seconds(), "error": err}.Log()
		return err
	}
	zipSize := b.Len()

	// compose and send the email
	msg := &postmarkapp.Message{
		From: &mail.Address{
			Name:    "Anki Accounts",
			Address: "accountmail@anki.com",
		},
		To:          []*mail.Address{{Address: targetEmail}},
		Subject:     "Profile Accounts Export",
		TextBody:    strings.NewReader(fmt.Sprintf("%d accounts exported as CSV attached.", count)),
		Attachments: []postmarkapp.Attachment{{Name: "users.csv.gz", Content: &b, ContentType: "application/x-gzip"}},
	}
	res, err := email.Emailer.PostmarkC.Send(msg)
	if err != nil || res.ErrorCode != 0 {
		alog.Error{"action": "email_csv", "status": "email_send_failed", "count": count, "runtime_seconds": runtime.Seconds(), "email_target": targetEmail, "error": err, "err_code": res.ErrorCode, "size_bytes": zipSize}.Log()
		return errors.New("Email failed to send")
	}
	alog.Info{"action": "email_csv", "status": "completed", "count": count, "runtime_seconds": runtime.Seconds(), "email_target": targetEmail, "size_bytes": zipSize}.Log()
	return nil
}
