// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// The user package provides API handlers for operations on user accounts.
package user

import (
	"fmt"
	"net/http"
	"strconv"
	"strings"

	"net/url"

	"github.com/anki/sai-accounts-audit/audit"
	"github.com/anki/sai-go-accounts/apierrors"
	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-accounts/models/user"
	autil "github.com/anki/sai-go-accounts/util"
	"github.com/anki/sai-go-accounts/validate"
	"github.com/anki/sai-go-util"
	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/http/apirouter"
	"github.com/anki/sai-go-util/http/simpleauth"
	"github.com/anki/sai-go-util/jsonutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/timedtoken"
	utilvalid "github.com/anki/sai-go-util/validate"
	"github.com/gorilla/mux"
)

const (
	maxJsonSize            = 64000
	DefaultSuggestionCount = 5
	MaxSuggestionCount     = 15
)

const (
	apiDeactivationReason = "deactivated-via-api"
	apiPurgeReason        = "purged-via-api"
)

var (
	// Fields accepted when creating a new account
	updateAccountWhitelist = jsonutil.NewWhitelist(
		"username", "password", "family_name", "given_name", "email", "drive_guest_id",
		"dob", "gender", "email_lang", "player_id")
	createAccountWhitelist = append(updateAccountWhitelist, jsonutil.NewWhitelist(
		"created_by_app_name", "created_by_app_version", "created_by_app_platform")...)

	handlers     []apirouter.Handler
	FormsUrlRoot string
)

func defineHandler(handler apirouter.Handler) {
	handlers = append(handlers, handler)
}

// RegisterAPI causes the session API handles to be registered with the http mux.
func RegisterAPI() {
	apirouter.RegisterHandlers(handlers)
}

func logValidationErrors(r *http.Request, ferrs utilvalid.FieldErrors) {
	if alog.MinLogLevel > alog.LogDebug {
		return
	}
	for _, ferr := range ferrs {
		for _, f := range ferr {
			alog.Debug{"action": "validate_error", "status": "bad_field", "field_name": f.FieldName, "code": f.Code, "message": f.Message, "value": f.Value}.LogR(r)
		}
	}
}

// Task Handlers

func init() {
	defineHandler(apirouter.Handler{
		Method: "GET,POST",
		Path:   "/1/tasks/users/deactivate-unverified",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.NoSession,
			SessionScopeMask: session.ScopeMask(session.CronScope),
			Handler:          user.UpdateUnverifiedRunner,
		}})
}

func init() {
	defineHandler(apirouter.Handler{
		Method: "GET,POST",
		Path:   "/1/tasks/users/purge-deactivated",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.NoSession,
			SessionScopeMask: session.ScopeMask(session.CronScope),
			Handler:          user.PurgeDeactivatedRunner,
		}})
}

func init() {
	defineHandler(apirouter.Handler{
		Method: "GET,POST",
		Path:   "/1/tasks/users/notify-unverified",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.NoSession,
			SessionScopeMask: session.ScopeMask(session.CronScope),
			Handler:          user.NotifyUnverifiedRunner,
		}})
}

func init() {
	defineHandler(apirouter.Handler{
		Method: "GET,POST",
		Path:   "/1/tasks/users/email-csv",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.NoSession,
			SessionScopeMask: session.ScopeMask(session.CronScope),
			Handler:          user.EmailCSVRunner,
		}})
}

func stringsToIf(in []string) []interface{} {
	result := make([]interface{}, len(in))
	for i := 0; i < len(in); i++ {
		result[i] = in[i]
	}
	return result
}

// Register the ListUsers endpoint
func init() {
	defineHandler(apirouter.Handler{
		Method: "GET",
		Path:   "/1/users",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.RequireSession,
			SessionScopeMask: session.AdminOrSupportScope,
			Handler:          jsonutil.JsonHandlerFunc(ListUsers),
		}})
}

const (
	queryLimitDefault = 50
)

type ListUsersResponse struct {
	Offset   int                 `json:"offset"`
	Accounts []*user.UserAccount `json:"accounts"`
}

// The ListUsers endpoint handles API requests to list user accounts
// Access is restricted to admin and support scopes
//
// GET /users
//
//TODO fields
func ListUsers(w jsonutil.JsonWriter, r *http.Request) {
	var err error
	var ListModifier user.AccountListModifier

	queryParams := r.URL.Query()

	// Handle order by parameter
	ListModifier.OrderBy = "username" // default to username
	if order := queryParams.Get("order_by"); order != "" {
		ListModifier.OrderBy = order
	}

	// sort ASC by default unless sort_descending parameter is set
	ListModifier.SortDesc = queryParams.Get("sort_descending") == "true"

	// Handle limit parameter
	ListModifier.Limit = queryLimitDefault
	if limit := queryParams.Get("limit"); limit != "" {
		ListModifier.Limit, err = strconv.Atoi(limit)
		if err != nil {
			w.JsonError(r, apierrors.NewFieldValidationError("The limit must be an integer"))
			return
		}
	}

	if statuses := queryParams.Get("status"); statuses != "" {
		for _, status := range strings.Split(statuses, ",") {
			switch status {
			case "active":
				ListModifier.Status = append(ListModifier.Status, user.AccountActive)
			case "deactivated":
				ListModifier.Status = append(ListModifier.Status, user.AccountDeactivated)
			case "purged":
				ListModifier.Status = append(ListModifier.Status, user.AccountPurged)
			default:
				w.JsonError(r, apierrors.NewFieldValidationError("account_status must be active, inactive, or purged"))
				return
			}
		}
	} else {
		ListModifier.Status = append(ListModifier.Status, user.AccountActive) // Only search active accounts by default
	}

	ListModifier.Offset = 0 // No offset by default
	if _offset := queryParams.Get("offset"); _offset != "" {
		ListModifier.Offset, err = strconv.Atoi(_offset)
		if err != nil {
			w.JsonError(r, apierrors.NewFieldValidationError("The offset must be an integer"))
			return
		}
	}

	// Handle result filters
	for _, filter := range user.ValidFilters {
		if filterVal := queryParams.Get(filter); filterVal != "" {
			f := user.FilterField{Name: filter, Value: filterVal}
			if filterMode := queryParams.Get(filter + "_mode"); filterMode != "" {
				f.Mode = filterMode
			} else {
				f.Mode = user.MatchFilter // Exact match by default
			}
			ListModifier.Filters = append(ListModifier.Filters, f)
		}
	}

	uas, err := user.ListAccounts(ListModifier)
	if err != nil {
		if err, ok := err.(utilvalid.FieldErrors); ok {
			logValidationErrors(r, err)
		}
		if err, ok := err.(jsonutil.JsonError); ok {
			alog.Error{"action": "list_users", "code": err.JsonError().HttpStatus, "status": err.JsonError().Code, "message": err.JsonError().Message}.LogR(r)
			w.JsonError(r, err)
			return
		}
		alog.Error{"action": "list_users", "status": "unexpected_error", "error": err}.LogR(r)
		w.JsonServerError(r)
		return
	}

	var resp ListUsersResponse

	if uas == nil {
		resp.Accounts = nil
		w.JsonOkData(r, resp)
		return
	}

	// if there was queryLimit results, provide an offset
	if len(uas) == ListModifier.Limit {
		resp.Offset = len(uas) + ListModifier.Offset
	}

	resp.Accounts = uas
	w.JsonOkData(r, resp)
}

// Register the CreateUser endpoint
func init() {
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/users",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.MaybeSession,
			SessionScopeMask: session.AnyScope,
			Handler:          jsonutil.JsonHandlerFunc(CreateUser),
		}})
}

// General handlers

// The CreateUser endpoint handles API requests to create a new user account.
//
// POST /users
//
func CreateUser(w jsonutil.JsonWriter, r *http.Request) {
	var (
		err  error
		jerr jsonutil.JsonError
	)

	data, jerr := jsonutil.ReadRequestJsonLimit(r, maxJsonSize)
	if jerr != nil {
		alog.Info{"action": "create_user", "status": "bad_json", "error": jerr}.LogR(r)
		w.JsonError(r, jerr)
		return
	}
	ua := &user.UserAccount{}
	if _, err := jsonutil.DecodeAndValidateJson(data, createAccountWhitelist, ua); err != nil {
		alog.Warn{"action": "create_user", "status": "validate_failed", "error": err}.LogR(r)
		w.JsonError(r, err)
		return
	}

	if ua.User == nil {
		alog.Info{"action": "create_user", "status": "no_user_data_supplied"}.LogR(r)
		w.JsonError(r, apierrors.NewFieldValidationError("No user data supplied"))
		return
	}

	// Set required fields for creation
	ua.User.Status = user.AccountActive

	// Avoid a DB update by setting EmailVerify manually
	ua.User.EmailVerify = autil.Pstring(timedtoken.Token())

	// PlayerId is an alias for DriveGuestId
	if id := autil.NS(ua.User.PlayerId); id != "" {
		if autil.NS(ua.User.DriveGuestId) != "" {
			w.JsonError(r, apierrors.PlayerIdConflict)
			return
		}
		ua.User.DriveGuestId = ua.User.PlayerId
	}

	// Commit the model.
	if err := user.CreateUserAccount(ua); err != nil {
		if err, ok := err.(utilvalid.FieldErrors); ok {
			logValidationErrors(r, err)
		}
		switch err := err.(type) {
		case jsonutil.JsonError:
			alog.Info{"action": "create_user", "status": "user_error", "error": err}.LogR(r)
			w.JsonError(r, err)
			return
		default:
			alog.Error{"action": "create_user", "status": "unexpected_error", "error": err}.LogR(r)
			w.JsonServerError(r)
			return
		}

	}

	// Create a session for the new user
	ua, err = user.AccountById(ua.User.Id) // fetch masked version
	if err != nil || ua == nil {
		var userId string
		if ua != nil {
			userId = ua.User.Id
		}
		alog.Error{"action": "create_user",
			"status":         "unexpected_error",
			"error":          "fetch_after_create_failed",
			"returned_error": err,
			"user_id":        userId}.LogR(r)
		w.JsonServerError(r)
		return
	}

	us, err := session.NewUserSession(ua)
	if err != nil {
		switch err := err.(type) {
		case jsonutil.JsonError:
			alog.Info{"action": "create_user_session", "status": "new_session_failed", "username": ua.User.Username, "error": err}.LogR(r)
			w.JsonError(r, err)
			return
		default:
			alog.Error{"action": "create_user_session", "status": "unexpected_error", "error": err}.LogR(r)
			w.JsonServerError(r)
			return
		}
	}

	go ua.SendVerificationEmail()

	audit.WriteRecord(&audit.Record{Action: audit.ActionAccountCreate, AccountID: ua.User.Id}, r)

	alog.Info{"action": "create_user", "status": "ok", "user_id": ua.User.Id}.LogR(r)
	w.JsonOkData(r, us)
}

// Register the UpdateUser API call
func init() {
	defineHandler(apirouter.Handler{
		Method: "PATCH",
		Path:   "/1/users/{userid}",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.RequireSession,
			SessionScopeMask: session.ValidSessionScope,
			Handler:          jsonutil.JsonHandlerFunc(UpdateUser),
		}})
}

// Add a "POST" based alias for the PATCH endpoint as Android apparently doesn't support PATCH!! :-(
func init() {
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/users/{userid}/patch",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.RequireSession,
			SessionScopeMask: session.ValidSessionScope,
			Handler:          jsonutil.JsonHandlerFunc(UpdateUser),
		}})
}

// The UpdateUser API handler makes modifications to an existing user account.
//
// PATCH /users/{userid}
//
// TODO: Add scope and ownership checks; trigger email notifications on email change
func UpdateUser(w jsonutil.JsonWriter, r *http.Request) {
	var err error
	userId := mux.Vars(r)["userid"]
	us := validate.ActiveSession(r)
	if userId == "me" {
		userId = us.User.Id
	}

	data, jerr := jsonutil.ReadRequestJsonLimit(r, maxJsonSize)
	if jerr != nil {
		w.JsonError(r, jerr)
		return
	}

	canUpdateAny := session.NewScopeMask(session.SupportScope, session.AdminScope, session.RootScope)
	if !canUpdateAny.Contains(us.Session.Scope) && us.User.Id != userId {
		alog.Warn{"action": "update_user", "status": "permission_denied",
			"user_id": us.User.Id, "message": "User tried to update another account"}.LogR(r)
		w.JsonError(r, apierrors.InvalidSessionPerms)
		return
	}

	var validKeys map[string]interface{}
	uaDelta := &user.UserAccount{}

	if validKeys, jerr = jsonutil.DecodeAndValidateJson(data, updateAccountWhitelist, uaDelta); jerr != nil {
		alog.Warn{"action": "update_user", "status": "validate_failed", "error": jerr, "user_id": userId}.LogR(r)
		w.JsonError(r, jerr)
		return
	}

	// Fetch the existing user
	ua, err := user.AccountById(userId)
	if err != nil {
		alog.Error{"action": "update_user", "status": "unexpected_error", "error": err, "user_id": userId}.LogR(r)
		w.JsonServerError(r)
		return
	}

	if ua == nil {
		alog.Debug{"action": "update_user", "status": "user_not_found", "user_id": userId}.LogR(r)
		w.JsonError(r, apierrors.AccountNotFound)
		return
	}

	if ua.Status != user.AccountActive {
		// only admins and support staff can update non-active accounts
		canUpdateNonActive := session.NewScopeMask(session.SupportScope, session.AdminScope, session.RootScope)
		if !canUpdateNonActive.Contains(us.Session.Scope) {
			w.JsonError(r, apierrors.AccountNotFound)
			return
		}
		if ua.Status == user.AccountPurged {
			// not even an admin can reactivate a purged account.
			w.JsonError(r, apierrors.CannotReactivatePurged)
			return
		}
	}

	// Update existing user using set fields
	// validKeys is used just for its keys for this so we only copy in the fields
	// that have actually been supplied in the request.
	oldUsername := ua.User.Username
	oldEmail := ua.User.Email
	var changedFields []string
	if uaDelta.User != nil {
		// PlayerId is an alias for DriveGuestId
		if id := autil.NS(uaDelta.User.PlayerId); id != "" {
			if gid := autil.NS(uaDelta.User.DriveGuestId); gid != "" && gid != id {
				w.JsonError(r, apierrors.PlayerIdConflict)
				return
			}
			uaDelta.User.DriveGuestId = uaDelta.User.PlayerId
			validKeys["drive_guest_id"] = true
		}
		changedFields = util.UpdateStructDelta(validKeys, uaDelta.User, ua.User)
	}

	if uaDelta.NewPassword != nil && *uaDelta.NewPassword != "" {
		ua.NewPassword = uaDelta.NewPassword
	}

	sendValidation := false
	if strings.EqualFold(*ua.User.Email, *oldEmail) != true {
		alog.Info{"action": "update_user", "status": "set_email_unverified", "user_id": userId}.LogR(r)
		ua.User.EmailVerify = autil.Pstring(timedtoken.Token())
		ua.User.EmailIsVerified = false
		ua.User.EmailFailureCode = nil
		sendValidation = true
	}

	if err = user.UpdateUserAccount(ua); err != nil {
		if err, ok := err.(jsonutil.JsonError); ok {
			// already has metadata
			alog.Info{"action": "update_user", "status": "error", "error": err, "user_id": userId}.LogR(r)
			w.JsonError(r, err)
			return
		}
		switch err := err.(type) {
		case jsonutil.JsonError:
			alog.Info{"action": "update_user", "status": "user_error", "old_username": oldUsername, "user_id": userId, "error": err}.LogR(r)
			w.JsonError(r, err)
		default:
			alog.Error{"action": "update_user", "status": "unexpected_error", "error": err, "user_id": userId}.LogR(r)
			w.JsonServerError(r)
		}
		return
	}

	// Check if password changed and take action/notify
	if uaDelta.NewPassword != nil && *uaDelta.NewPassword != "" {
		// invalidate all sessions except the current one after password is updated
		if err := session.DeleteOtherByUserId(userId, us.Session); err != nil {
			// always in an internal error; log only
			alog.Error{"action": "update_user", "status": "session_delete_failed", "error": err, "user_id": userId}.LogR(r)
		} else {
			alog.Info{"action": "update_user", "status": "password_changed", "user_id": userId}.LogR(r)
		}

		// Notify the user that their account changed materially (done only for password right now)
		go ua.SendAccountUpdatedEmail()
	}

	if sendValidation {
		go ua.SendVerificationEmail()
	}

	audit.WriteRecord(&audit.Record{Action: audit.ActionAccountUpdate, AccountID: userId}, r)

	alog.Info{"action": "update_user", "status": "ok", "user_id": userId, "updated_fields": strings.Join(changedFields, ",")}.LogR(r)

	w.JsonOkData(r, ua)
}

// Register the GetUser API call
func init() {
	defineHandler(apirouter.Handler{
		Method: "GET",
		Path:   "/1/users/{userid}",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.MaybeSession,
			SessionScopeMask: session.ValidSessionScope,
			Handler:          jsonutil.JsonHandlerFunc(GetUser),
		}})
}

type restrictedUserView struct {
	Status user.AccountStatus `json:"status"`
}

type restrictedAdminView struct {
	RemoteId string `json:"remote_id"`
}

// The GetUser API handler fetches information on a single user.
//
// GET /users/{userid}
//
// TODO Check fields returned against whitelist
func GetUser(w jsonutil.JsonWriter, r *http.Request) {
	var err error
	userId := mux.Vars(r)["userid"]
	us := validate.ActiveSession(r)

	// "me" means the userId of the owner of the active session, if there is one
	if userId == "me" {
		if us == nil {
			// avoid the db hit if there's no session and "me" is meaningless
			w.JsonError(r, apierrors.AccountNotFound)
			return
		}
		if us.User == nil {
			// Admin session
			w.JsonOkData(r, restrictedAdminView{us.Session.RemoteId})
			return
		}
		userId = us.User.Id
	}

	ua, err := user.AccountById(userId)
	if err != nil {
		alog.Error{"action": "get_user", "status": "error", "message": err, "user_id": userId}.LogR(r)
		w.JsonServerError(r)
		return
	}

	if ua == nil {
		w.JsonError(r, apierrors.AccountNotFound)
		return
	}

	// Do not audit account access if the account being accessed
	// belongs to the active session - we only want to audit accesses
	// when a user or service accesses an account they do NOT own.
	if us == nil || us.User == nil || (userId != us.User.Id) {
		audit.WriteRecord(&audit.Record{Action: audit.ActionAccountAccess, AccountID: userId}, r)
	}

	canViewAny := session.NewScopeMask(session.SupportScope, session.AdminScope, session.RootScope)
	if us == nil || (!canViewAny.Contains(us.Session.Scope) && us.User.Id != userId) {
		// either no session or the user is accessing another account's details
		// either way the information returned is limited
		w.JsonOkData(r, restrictedUserView{ua.Status})
	} else {
		w.JsonOkData(r, ua)
	}
}

func init() {
	defineHandler(apirouter.Handler{
		Method: "GET",
		Path:   "/1/users/{userid}/related",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.NoSession,
			SessionScopeMask: session.ScopeMask(session.ServiceScope),
			Handler:          jsonutil.JsonHandlerFunc(RelatedUsers),
		}})
}

// RelatedUsers returns user ids that share an email address with the requested user id
func RelatedUsers(w jsonutil.JsonWriter, r *http.Request) {
	const action = "related_users"
	var err error
	userId := mux.Vars(r)["userid"]

	ua, err := user.AccountById(userId)
	if err != nil {
		alog.Error{"action": action, "status": "error", "message": err, "user_id": userId}.LogR(r)
		w.JsonServerError(r)
		return
	}

	if ua == nil || ua.User.Status != user.AccountActive {
		alog.Info{"action": action, "status": "bad_request", "user_id": userId,
			"reason": "account_inactive_or_invalid"}.LogR(r)
		w.JsonError(r, apierrors.AccountNotFound)
		return
	}

	users, err := ua.Related()
	if err != nil {
		alog.Error{"action": action, "status": "error", "message": err, "user_id": userId}.LogR(r)
		w.JsonServerError(r)
		return
	}

	result := make([]string, len(users))

	for i, ua := range users {
		result[i] = ua.Id
	}

	alog.Info{"action": action, "status": "ok", "user_id": userId,
		"related_count": len(users), "related_ids": strings.Join(result, ",")}.LogR(r)

	audit.WriteRecord(&audit.Record{Action: audit.ActionAccountGetRelated, AccountID: ua.User.Id}, r)

	w.JsonOkData(r, map[string]interface{}{
		"user_ids": result,
	})
}

// Register the DeactivateUser AIP call
func init() {
	defineHandler(apirouter.Handler{
		Method: "DELETE",
		Path:   "/1/users/{userid}",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.RequireSession,
			SessionScopeMask: session.ValidSessionScope,
			Handler:          jsonutil.JsonHandlerFunc(DeactivateUser),
		}})
}

// The DeactivateUser API handler marks a user account as deactivated.
//
// Users with Support or Admin scope may deactivate any account
//
// DELETE /users/{userid}
func DeactivateUser(w jsonutil.JsonWriter, r *http.Request) {
	var err error
	userId := mux.Vars(r)["userid"]
	us := validate.ActiveSession(r)
	if userId == "me" {
		userId = us.User.Id
	}

	canDeleteAny := session.NewScopeMask(session.SupportScope, session.AdminScope, session.RootScope)
	if !canDeleteAny.Contains(us.Session.Scope) && us.User.Id != userId {
		alog.Warn{"action": "deactivate_user", "status": "permission_denied",
			"user_id": us.User.Id, "message": "User tried to delete another account"}.LogR(r)
		w.JsonError(r, apierrors.InvalidSessionPerms)
		return
	}

	ua, err := user.AccountById(userId)
	if err != nil {
		alog.Error{"action": "deactivate_user", "status": "error", "message": err, "user_id": userId}.LogR(r)
		w.JsonServerError(r)
		return
	}

	if ua == nil {
		w.JsonError(r, apierrors.AccountNotFound)
		return
	}

	if err := user.DeactivateUserAccount(userId, apiDeactivationReason); err != nil {
		alog.Error{"action": "deactivate_user", "status": "unexpected_error", "error": err, "user_id": userId}.LogR(r)
		w.JsonServerError(r)
		return
	}

	if err := session.DeleteByUserId(userId); err != nil {
		// always in an internal error; log only
		alog.Error{"action": "deactivate_uesr", "status": "session_delete_failed", "error": err, "user_id": userId}.LogR(r)
	}

	audit.WriteRecord(&audit.Record{Action: audit.ActionAccountDeactivate, AccountID: userId}, r)

	alog.Info{"action": "deactivate_user", "status": "ok", "user_id": userId}.LogR(r)
	w.JsonOk(r, "Account deactivated")
}

// Register the ReactivateUser AIP call
func init() {
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/users/{userid}/reactivate",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.RequireSession,
			SessionScopeMask: session.AdminOrSupportScope,
			Handler:          jsonutil.JsonHandlerFunc(ReactivateUser),
		}})
}

// The ReactivateUser API handler reactiavtes a previously deactivated account
//
// Only available to users with Support or Admin scope
//
// POST /users/{userid}/reactivate

func ReactivateUser(w jsonutil.JsonWriter, r *http.Request) {
	userId := mux.Vars(r)["userid"]

	username := r.PostFormValue("username")
	if username == "" {
		w.JsonError(r, apierrors.MissingParameter)
		return
	}

	if err := user.ReactivateUserAccount(userId, username); err != nil {
		switch err := err.(type) {
		case jsonutil.JsonError:
			alog.Info{"action": "reactivate_user", "status": "user_error", "user_id": userId, "error": err}.LogR(r)
			w.JsonError(r, err)
			return
		default:
			alog.Error{"action": "reactivate_user", "status": "unexpected_error", "error": err, "user_id": userId}.LogR(r)
			w.JsonServerError(r)
			return
		}
	}

	audit.WriteRecord(&audit.Record{Action: audit.ActionAccountReactivate, AccountID: userId}, r)

	alog.Info{"action": "reactivate_user", "status": "ok", "user_id": userId}.LogR(r)
	w.JsonOk(r, "Account reactivated")
}

// Register the PurgeUser API call
func init() {
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/users/{userid}/purge",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.RequireSession,
			SessionScopeMask: session.AdminOrSupportScope,
			Handler:          jsonutil.JsonHandlerFunc(PurgeUser),
		}})
}

// The PurgeUser API handler clears all Personally Identifiable Information
// from a deactivated user account.
//
// POST /users/{userid}/purge
func PurgeUser(w jsonutil.JsonWriter, r *http.Request) {
	userId := mux.Vars(r)["userid"]

	if err := user.PurgeUserAccount(userId, apiPurgeReason); err != nil {
		switch err := err.(type) {
		case jsonutil.JsonError:
			alog.Info{"action": "purge_user", "status": "user_error", "user_id": userId, "error": err}.LogR(r)
			w.JsonError(r, err)
		default:
			alog.Error{"action": "purge_user", "status": "unexpected_error", "error": err, "user_id": userId}.LogR(r)
			w.JsonServerError(r)
		}
		return
	}

	audit.WriteRecord(&audit.Record{Action: audit.ActionAccountPurge, AccountID: userId}, r)

	alog.Info{"action": "purge_user", "status": "ok", "user_id": userId}.LogR(r)
	w.JsonOk(r, "Account purged")
}

// TODO: Should this really be AnyScope?
func init() {
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/users/{userid}/verify_email",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.RequireSession,
			SessionScopeMask: session.AnyScope,
			Handler:          jsonutil.JsonHandlerFunc(VerifyEmail),
		}})
}

// VerifyEmail Requests that a new verification link be sent to the email address associated with the account.
// This requests invalidates any previous email validation tokens for the same account.
// TODO: Are these tokens really supposed to be for 7 days? I have it set to 1 day
func VerifyEmail(w jsonutil.JsonWriter, r *http.Request) {
	userId := mux.Vars(r)["userid"]

	ua, err := user.AccountById(userId)
	if err != nil {
		alog.Error{"action": "verify_email", "status": "error", "message": err, "user_id": userId}.LogR(r)
		w.JsonServerError(r)
		return
	}
	if ua == nil || ua.User.Status != user.AccountActive {
		w.JsonError(r, apierrors.AccountNotFound)
		return
	}
	if ua.User.EmailIsVerified == true {
		alog.Info{"action": "verify_email", "status": "email_already_verified", "user_id": userId}.LogR(r)
		w.JsonError(r, apierrors.EmailAlreadyVerified)
		return
	}
	err = ua.SetEmailValidationToken(timedtoken.Token())
	if err != nil {
		switch err := err.(type) {
		case jsonutil.JsonError:
			alog.Info{"action": "verify_email", "status": "user_error", "userid": userId, "error": err}.LogR(r)
			w.JsonError(r, err)
			return
		default:
			alog.Error{"action": "verify_email", "status": "unexpected_error", "error": err, "user_id": userId}.LogR(r)
			w.JsonServerError(r)
			return
		}
	}
	go ua.SendVerificationEmail()
	alog.Info{"action": "verify_email", "status": "sent", "user_id": userId}.LogR(r)
	w.JsonOk(r, "Verification email sent")
}

func init() {
	defineHandler(apirouter.Handler{
		Method: "GET",
		Path:   "/1/users/{userid}/verify_email_confirm/{token}",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.NoSession,
			SessionScopeMask: session.AnyScope,
			Handler:          jsonutil.JsonHandlerFunc(VerifyEmailConfirm),
			NoAppKey:         true,
		}})
}

// VerifyEmailConfirm is the endpoint the user reaches by clicking on the link in the email verification message.
// This sets their email to verified.
// It also replies with a 302 redirect to a page that indicates success or failure.
func VerifyEmailConfirm(w jsonutil.JsonWriter, r *http.Request) {
	userId := mux.Vars(r)["userid"]
	token := mux.Vars(r)["token"]
	fnc := "verify_email_confirm"
	okurl := fmt.Sprintf("%s/%s", FormsUrlRoot, "email-validation-ok")
	errurl := fmt.Sprintf("%s/%s", FormsUrlRoot, "email-validation-error")

	if token == "" {
		alog.Info{"action": fnc, "status": "no_token", "user_id": userId, "error": "No Token Supplied"}.LogR(r)
		http.Redirect(w, r, errurl, http.StatusFound)
		return
	}

	ua, err := user.AccountById(userId)
	if err != nil {
		alog.Error{"action": fnc, "status": "unexpected_error", "user_id": userId, "error": err}.LogR(r)
		http.Redirect(w, r, errurl, http.StatusFound)
		return
	}

	if ua == nil || ua.User.Status != user.AccountActive {
		alog.Info{"action": fnc, "status": "active_user_not_found", "user_id": userId}.LogR(r)
		http.Redirect(w, r, errurl, http.StatusFound)
		return
	}

	// Complete OK URL here
	switch autil.NS(ua.CreatedByAppName) {
	case "drive", "overdrive", "foxtrot":
		okurl = fmt.Sprintf("%s-app", okurl)
	case "web", "salesforce", "sfcc":
		okurl = fmt.Sprintf("%s-web", okurl)
	default:
		alog.Warn{"action": fnc, "status": "CreateByAppName not known for user, defaulting to web", "user_id": userId, "app_name": ua.CreatedByAppName}.LogR(r)
		okurl = fmt.Sprintf("%s-web", okurl)
	}

	isChild, _ := ua.IsChild()
	if isChild {
		okurl = fmt.Sprintf("%s-parent", okurl)
	}

	// Add user-specific parameters to url
	u, err := url.Parse(okurl)
	if err != nil {
		alog.Error{"action": fnc, "status": "ok_url_parse", "user_id": userId, "error": err}.LogR(r)
	} else {
		astr := "T"
		if isChild {
			astr = "F"
		}
		q := u.Query()
		q.Set("u", autil.NS(ua.Username))
		q.Set("e", autil.NS(ua.Email))
		q.Set("s", autil.NS(ua.CreatedByAppName))
		q.Set("a", astr)
		u.RawQuery = q.Encode()
		okurl = u.String()
	}

	if ua.EmailIsVerified {
		alog.Info{"action": fnc, "status": "email_already_verified", "user_id": userId}.LogR(r)
		http.Redirect(w, r, okurl, http.StatusFound)
		return
	}

	if ua.IsEmailVerify(token) == true {
		err = ua.SetEmailValidated()
		if err != nil {
			alog.Error{"action": fnc, "status": "unexpected_error_on_verify", "error": err, "user_id": userId}.LogR(r)
			http.Redirect(w, r, errurl, http.StatusFound)
			return
		} else {
			go ua.SendPostVerificationEmail()
			http.Redirect(w, r, okurl, http.StatusFound)
			return
		}
	}

	alog.Info{"action": fnc, "status": "invalid_token", "error": err, "user_id": userId}.LogR(r)
	http.Redirect(w, r, errurl, http.StatusFound)
	return
} // end VerifyEmailConfirm

func init() {
	defineHandler(apirouter.Handler{
		Method: "GET",
		Path:   "/1/usernames/{username}",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.MaybeSession,
			SessionScopeMask: session.AnyScope,
			Handler:          jsonutil.JsonHandlerFunc(IsUserAvailable),
		}})
}

// The IsUserAvaialble API handler verifies whether a username is in use.
//
// GET /usernames/{username}
// TODO: restrict respone data by scope
func IsUserAvailable(w jsonutil.JsonWriter, r *http.Request) {
	var err error
	username := mux.Vars(r)["username"]
	u, err := user.UserByUsername(username)
	if err != nil {
		alog.Error{"action": "is_user_available", "status": "error", "message": err, "username": username}.LogR(r)
		w.JsonServerError(r)
		return
	}

	if u == nil {
		w.JsonError(r, apierrors.AccountNotFound)
		return
	}

	UsernameExistsResponse(w, r, u)
}

// Define the YetAnotherIsUserAvailable endpoint
func init() {
	defineHandler(apirouter.Handler{
		Method: "GET",
		Path:   "/1/username-check",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.MaybeSession,
			SessionScopeMask: session.AnyScope,
			Handler:          jsonutil.JsonHandlerFunc(YetAnotherIsUserAvailable),
		}})
}

// The YetAnotherIsUserAvailable API handler does the same thing as the IsUserAvailable endpoint.
// It just takes username as a query string argument and always returns a 200 result.  Icky.
func YetAnotherIsUserAvailable(w jsonutil.JsonWriter, r *http.Request) {
	var err error
	usernames := r.URL.Query()["username"]
	if len(usernames) < 1 {
		w.JsonError(r, apierrors.MissingParameter)
		return
	}
	username := usernames[0]

	u, err := user.UserByUsername(username)
	if err != nil {
		alog.Error{"action": "is_user_available", "status": "error", "message": err, "username": username}.LogR(r)
		w.JsonServerError(r)
		return
	}

	if u != nil {
		UsernameExistsResponse(w, r, u)
	} else {
		w.JsonOkData(r, map[string]interface{}{"exists": false})
	}
}

// Formulate response to username existence checks based on app key scope
// Admin and Support scopes include user ID and user status
// Other scopes only report that the account exists
func UsernameExistsResponse(w jsonutil.JsonWriter, r *http.Request, u *user.User) {
	var response = make(map[string]interface{})
	ks := validate.ActiveApiKey(r)
	supportOrAdmin := session.NewScopeMask(session.SupportScope, session.AdminScope, session.RootScope)
	if ks != nil && supportOrAdmin.Contains(session.Scope(ks.Scopes)) {
		response["user_id"] = u.Id
		status, err := u.Status.MarshalText()
		if err != nil {
			alog.Error{"action": "convert_user_status", "status": "error", "message": err}.LogR(r)
		}
		response["status"] = string(status)
	}
	response["exists"] = true
	w.JsonOkData(r, response)
}

func init() {
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/suggestions",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.MaybeSession,
			SessionScopeMask: session.AnyScope,
			Handler:          jsonutil.JsonHandlerFunc(Suggestions),
		}})
}

// The Suggestions API handler takes a username and returns a list of alternate usernames
// that aren't already in use.
//
// GET /suggestions
func Suggestions(w jsonutil.JsonWriter, r *http.Request) {
	var err error

	username := r.PostFormValue("username")
	if username == "" {
		w.JsonError(r, apierrors.MissingParameter)
		return
	}

	count := DefaultSuggestionCount
	if c := r.PostFormValue("count"); c != "" {
		count, err = strconv.Atoi(c)
		if err != nil || count < 1 || count > MaxSuggestionCount {
			alog.Info{"action": "suggestions", "status": "bad_input", "error": "Invalid integer supplied for count parameter"}.LogR(r)
			w.JsonError(
				r, apierrors.NewFieldValidationError(
					fmt.Sprintf("Invalid value for count: Must be integer between 1 and %d", MaxSuggestionCount)))
			return
		}
	}

	var suggestions []string

	more, err := user.GenerateSuggestions(username, count)
	if err != nil {
		switch err := err.(type) {
		case jsonutil.JsonError:
			alog.Info{"action": "suggestions", "status": "user_error", "username": username, "count": count, "error": err}.LogR(r)
			w.JsonError(r, err)
			return
		default:
			alog.Error{"action": "suggestions", "status": "unexpected_error", "username": username, "count": count, "error": err}.LogR(r)
			w.JsonServerError(r)
			return
		}
	}
	suggestions = append(suggestions, more...)

	w.JsonOkData(r, map[string]interface{}{
		"suggestions": suggestions,
	})
}

func init() {
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/reset_user_password",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.MaybeSession,
			SessionScopeMask: session.AnyScope,
			Handler:          jsonutil.JsonHandlerFunc(ResetUserPassword),
		}})
}

// The reset_user_password API handler takes a username or an email address.
// It will send a password reset email to a user which provides links to reset passwords for all accounts
// that can be found with the data. If email, this could be multiple accounts which share that email.
//
// POST /1/reset_user_password
// TODO: Handle the case where the user email is not validated
func ResetUserPassword(w jsonutil.JsonWriter, r *http.Request) {
	formemail := r.PostFormValue("email")
	username := r.PostFormValue("username")
	userid := r.PostFormValue("userid")

	var (
		mode      string
		modeParam string
		uas       []*user.UserAccount
		err       error
	)

	switch {
	case userid != "":
		uas, err = user.SendForgotPasswordByUserId(userid)
		mode = "userid"
		modeParam = userid

	case username != "":
		uas, err = user.SendForgotPasswordByUsername(username)
		mode = "username"
		modeParam = username

	case formemail != "":
		uas, err = user.SendForgotPasswordByEmail(formemail)
		mode = "email"
		modeParam = formemail
	default:
		alog.Info{"action": "resetuserpassword", "status": "no_valid_param"}.LogR(r)
		w.JsonError(r, apierrors.AccountNotFound)
		return
	}

	if err != nil {
		switch err := err.(type) {
		case jsonutil.JsonError:
			alog.Info{"action": "resetuserpassword", "mode": mode, "modeparam": modeParam, "status": "user_error", "error": err}.LogR(r)
			w.JsonError(r, err)
			return
		default:
			alog.Error{"action": "resetuserpassword", "mode": mode, "modeparam": modeParam, "status": "unexpected_error", "error": err}.LogR(r)
			w.JsonServerError(r)
			return
		}
	}

	var userIds []string
	for _, ua := range uas {
		userIds = append(userIds, ua.Id)
	}

	for _, id := range userIds {
		audit.WriteRecord(&audit.Record{Action: audit.ActionAccountResetPassword, AccountID: id}, r)
	}

	alog.Info{"action": "resetuserpassword", "mode": mode, "modeparam": modeParam, "users": strings.Join(userIds, ","), "status": "sent"}.LogR(r)
	w.JsonOk(r, "Reset password email sent")
}

func init() {
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/reset_password_confirm",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.NoSession,
			SessionScopeMask: session.AnyScope,
			Handler:          jsonutil.JsonHandlerFunc(ResetPasswordConfirm),
			NoAppKey:         true,
		}})
}

// ResetPasswordConfirm is the endpoint the user reaches by submitting the reset password form.
// This sets their email to verified, and updates their password.
// It also replies with a 302 redirect to a page that indicates success or failure.
func ResetPasswordConfirm(w jsonutil.JsonWriter, r *http.Request) {
	fnc := "reset_password_confirm"
	token := r.PostFormValue("token")
	userId := r.PostFormValue("user_id")
	password := r.PostFormValue("password")

	okurl := fmt.Sprintf("%s/%s", FormsUrlRoot, "password-reset-ok")
	errurl := fmt.Sprintf("%s/%s", FormsUrlRoot, "password-reset-error")

	if token == "" {
		alog.Info{"action": fnc, "status": "no_token", "user_id": userId, "error": "No Token Supplied"}.LogR(r)
		http.Redirect(w, r, fmt.Sprintf("%s?err=no_token", errurl), http.StatusFound)
		return
	}

	if userId == "" {
		alog.Info{"action": fnc, "status": "no_user_id", "error": "No user_Id Supplied"}.LogR(r)
		http.Redirect(w, r, fmt.Sprintf("%s?err=no_user_id", errurl), http.StatusFound)
		return
	}

	if password == "" {
		alog.Info{"action": fnc, "status": "no_password", "user_id": userId, "error": "No password Supplied"}.LogR(r)
		http.Redirect(w, r, fmt.Sprintf("%s?err=no_password", errurl), http.StatusFound)
		return
	}

	ua, err := user.AccountById(userId)
	if err != nil {
		alog.Error{"action": fnc, userId: userId, "status": "unexpected_error", "error": err}.LogR(r)
		http.Redirect(w, r, fmt.Sprintf("%s?err=db_error", errurl), http.StatusFound)
		return
	}

	if ua == nil {
		alog.Debug{"action": fnc, "status": "user_not_found", "user_id": userId}.LogR(r)
		http.Redirect(w, r, fmt.Sprintf("%s?err=user_not_found", errurl), http.StatusFound)
		return
	}

	if err = ua.PasswordUpdateByToken(token, password); err != nil {
		switch err {
		case user.InvalidPasswordToken:
			alog.Info{"action": fnc, "status": "invalid_token", "user_id": userId, "token": token, "error": err}.LogR(r)
			http.Redirect(w, r, fmt.Sprintf("%s?err=invalid_token", errurl), http.StatusFound)

		default:
			alog.Error{"action": fnc, "status": "Error Updating Password By Token", "user_id": userId, "error": err}.LogR(r)
			http.Redirect(w, r, fmt.Sprintf("%s?err=err_pw_update", errurl), http.StatusFound)
		}
		return
	}

	// invalidate all sessions after password is updated
	if err := session.DeleteByUserId(userId); err != nil {
		// always in an internal error; log only
		alog.Error{"action": fnc, "status": "session_delete_failed", "user_id": userId, "error": err}.LogR(r)
	} else {
		alog.Info{"action": fnc, "status": "password_changed", "user_id": userId}.LogR(r)
	}

	go ua.SendAccountUpdatedEmail()

	http.Redirect(w, r, okurl, http.StatusFound)
	return
}

var PostmarkHookAuthUsers = map[string]string{}

var principal = &audit.Principal{
	Type:  audit.TypeRemote,
	Scope: audit.PrincipalScope(session.ScopeNames[session.ServiceScope]),
	ID:    buildinfo.Info().Product,
}

func init() {
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/postmark_bounce_webhook",
		Handler: &simpleauth.Handler{
			Users:   PostmarkHookAuthUsers,
			Handler: jsonutil.JsonHandlerFunc(PostmarkBounceWebhook),
		},
	})
}

type pmBounceData struct {
	Type        string
	Email       string
	Subject     string
	MessageID   string
	Description string
	BouncedAt   string
}

func PostmarkBounceWebhook(w jsonutil.JsonWriter, r *http.Request) {
	var bounce pmBounceData
	if jerr := jsonutil.ParseRequestJsonLimit(r, maxJsonSize, &bounce); jerr != nil {
		alog.Warn{"action": "postmark_bounce_webhook", "status": "bad_json", "error": jerr}.LogR(r)
		w.JsonError(r, jerr)
		return
	}

	switch bounce.Type {
	// match hard errors with no action (Go does not passthru by default)
	case "HardBounce":
	case "SpamNotification":
	case "SpamComplaint":
	case "BadEmailAddress":
	case "ManuallyDeactivated":
	default:
		alog.Info{"action": "postmark_bounce_webhook", "status": "soft_bounce",
			"email": bounce.Email, "message_id": bounce.MessageID, "subject": bounce.Subject,
			"desc": bounce.Description, "bounce_time": bounce.BouncedAt}.LogR(r)
		w.JsonOk(r, "Bounce logged")
		return
	}

	alog.Info{"action": "postmark_bounce_webhook", "status": "hard_bounce",
		"email": bounce.Email, "message_id": bounce.MessageID, "subject": bounce.Subject,
		"desc": bounce.Description, "bounce_time": bounce.BouncedAt}.LogR(r)

	accounts, err := user.AccountsByEmail(bounce.Email)
	if err == nil {
		for _, account := range accounts {
			audit.WriteRecord(&audit.Record{
				Principal: principal,
				AccountID: account.User.Id,
				Action:    audit.ActionAccountEmailBounced,
				Level:     audit.LevelError,
				Data: map[string]interface{}{
					"bounce": bounce,
				},
			}, r)
		}
	}

	if err := user.MarkAddressAsFailed(bounce.Email, "address_bounced"); err != nil {
		alog.Warn{"action": "postmark_bounce_webhook", "status": "db_failure", "error": err}.LogR(r)
		w.JsonServerError(r)
	} else {
		w.JsonOk(r, "Bounce logged")
	}
}
