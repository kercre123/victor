// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package testutil

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"
	"strconv"
	"strings"
)

var (
	NotFound  = errors.New("Entry not found")
	NotString = errors.New("Entry was not a string")
)

// Wrap a json response with some utility methods
type Json map[string]interface{}

// Field fetches a subkey from a field
// eg to access response["session"]["session_token"] safely use
// response.Field("session", "session_token")
// returns nil if the key doesn't exist
func (j Json) Field(subkeys ...string) (interface{}, error) {
	var m interface{} = map[string]interface{}(j)
	var ok bool
	for _, key := range subkeys {
		m, ok = m.(map[string]interface{})[key]
		if !ok {
			return nil, NotFound
		}
	}
	return m, nil
}

// FieldStr returns a subkey from a field as a string
// It will return a NotString error if the leaf isn't a string
// or a NotFound error if the item doesn't exist
func (j Json) FieldStr(subkeys ...string) (string, error) {
	result, err := j.Field(subkeys...)
	if err != nil {
		return "", err
	}
	if result, ok := result.(string); ok {
		return result, nil
	}
	return "", NotString
}

// FieldStrQ returns a subkey from a field as a string.
// It will return the empty string if the field doesn't exist,
// or isn't a string.
func (j Json) FieldStrQ(subkeys ...string) (result string) {
	result, _ = j.FieldStr(subkeys...)
	return result
}

// The Request type provides a convenient way of building common HTTP requests.
type Request struct {
	Method       string
	Url          string
	JsonData     Json
	FormData     url.Values
	AppKey       string
	SessionToken string
	Header       http.Header
}

func (r *Request) Request() *http.Request {
	req, _ := http.NewRequest(r.Method, r.Url, nil)
	if r.JsonData != nil {
		j, err := json.Marshal(r.JsonData)
		if err != nil {
			panic("Bad json data supplied to NewRequest: " + err.Error())
		}
		reader := bytes.NewReader(j)
		req.Body = ioutil.NopCloser(reader)
		req.ContentLength = int64(reader.Len())
		req.Header.Set("Content-type", "text/json")

	} else if r.FormData != nil {
		reader := strings.NewReader(r.FormData.Encode())
		req.Body = ioutil.NopCloser(reader)
		req.ContentLength = int64(reader.Len())
		req.Header.Set("Content-type", "application/x-www-form-urlencoded")
	}

	if r.AppKey != "" {
		req.Header.Set("Anki-App-Key", r.AppKey)
	}
	if r.SessionToken != "" {
		req.Header.Set("Authorization", "Anki "+r.SessionToken)
	}
	for k, v := range r.Header {
		req.Header[k] = v
	}
	return req
}

func (r *Request) Do() (*http.Response, error) {
	return http.DefaultClient.Do(r.Request())
}

func (r *Request) DoJsonResponse() (resp *http.Response, jdata Json, err error) {
	resp, err = http.DefaultClient.Do(r.Request())
	if resp != nil {
		defer resp.Body.Close()
		body, _ := ioutil.ReadAll(resp.Body)
		err = json.Unmarshal(body, &jdata)
		if err != nil {
			err = fmt.Errorf("Error from json decoder: %s - data=%q", err, string(body))
		}
		return resp, jdata, err
	}
	return resp, nil, err
}

type AccountsClient struct {
	ServerUrl    string
	SessionToken string
	AppKey       string
}

// rate limits will apply to all requests made with this key
func (c *AccountsClient) RateLimitedKey() string {
	return "rate-limited-key"
}

func (c *AccountsClient) NewRequest(method, urlStr string, jsonData map[string]interface{}, form url.Values) *Request {
	r := &Request{
		Method:       method,
		Url:          c.ServerUrl + urlStr,
		JsonData:     Json(jsonData),
		FormData:     form,
		AppKey:       c.AppKey,
		SessionToken: c.SessionToken,
	}
	return r
}

// purge deactivated users
func (c *AccountsClient) PurgeDeactivated(maxIdle string) (response *http.Response, jdata Json, err error) {
	frm := url.Values{
		"max-idle": {maxIdle},
	}
	r := c.NewRequest("POST", "/1/tasks/users/purge-deactivated", nil, frm)
	return r.DoJsonResponse()
}

// status of purge deactivated users task
func (c *AccountsClient) PurgeDeactivatedStatus() (response *http.Response, jdata Json, err error) {
	r := c.NewRequest("GET", "/1/tasks/users/purge-deactivated", nil, nil)
	return r.DoJsonResponse()
}

func (c *AccountsClient) DeactivateUnverified(maxIdle string) (response *http.Response, jdata Json, err error) {
	frm := url.Values{
		"max-idle": {maxIdle},
	}
	r := c.NewRequest("POST", "/1/tasks/users/deactivate-unverified", nil, frm)
	return r.DoJsonResponse()
}

func (c *AccountsClient) DeactivateUnverifiedStatus() (response *http.Response, jdata Json, err error) {
	r := c.NewRequest("GET", "/1/tasks/users/deactivate-unverified", nil, nil)
	return r.DoJsonResponse()
}

func (c *AccountsClient) NotifyUnverified(notifyOlderThan string) (response *http.Response, jdata Json, err error) {
	frm := url.Values{
		"notify-older-than": {notifyOlderThan},
	}
	r := c.NewRequest("POST", "/1/tasks/users/notify-unverified", nil, frm)
	return r.DoJsonResponse()
}

func (c *AccountsClient) NotifyUnverifiedStatus() (response *http.Response, jdata Json, err error) {
	r := c.NewRequest("GET", "/1/tasks/users/notify-unverified", nil, nil)
	return r.DoJsonResponse()
}

func (c *AccountsClient) ReactivateUser(userId string, username string) (response *http.Response, jdata Json, err error) {
	frm := url.Values{
		"username": {username},
	}
	r := c.NewRequest("POST", "/1/users/"+userId+"/reactivate", nil, frm)
	return r.DoJsonResponse()
}

// Fetch a user, by userid.
func (c *AccountsClient) UserById(userId string) (response *http.Response, jdata Json, err error) {
	r := c.NewRequest("GET", "/1/users/"+userId, nil, nil)
	return r.DoJsonResponse()
}

// Create a new user.
func (c *AccountsClient) CreateUser(userInfo map[string]interface{}) (response *http.Response, jdata Json, err error) {
	r := c.NewRequest("POST", "/1/users", userInfo, nil)
	return r.DoJsonResponse()
}

// Update existing user
func (c *AccountsClient) UpdateUser(userId string, userInfo map[string]interface{}) (response *http.Response, jdata Json, err error) {
	r := c.NewRequest("PATCH", "/1/users/"+userId, userInfo, nil)
	return r.DoJsonResponse()
}

// Delete (deactivate) a user account.
func (c *AccountsClient) DeleteUser(userId string) (response *http.Response, jdata Json, err error) {
	r := c.NewRequest("DELETE", "/1/users/"+userId, nil, nil)
	return r.DoJsonResponse()
}

// Login to a user account using a username & password.
func (c *AccountsClient) NewUserSession(userName, password string) (response *http.Response, jdata Json, err error) {
	frm := url.Values{
		"username": {userName},
		"password": {password},
	}
	r := c.NewRequest("POST", "/1/sessions", nil, frm)
	return r.DoJsonResponse()
}

func (c *AccountsClient) NewUserIdSession(userId, password string) (response *http.Response, jdata Json, err error) {
	frm := url.Values{
		"userid":   {userId},
		"password": {password},
	}
	r := c.NewRequest("POST", "/1/sessions", nil, frm)
	return r.DoJsonResponse()
}

// Reup a session using an existing session token
func (c *AccountsClient) ReupSession() (response *http.Response, jdata Json, err error) {
	r := c.NewRequest("POST", "/1/sessions/reup", nil, nil)
	return r.DoJsonResponse()
}

// Delete a session
func (c *AccountsClient) DeleteSession(sessionToken string, deleteRelated bool) (response *http.Response, jdata Json, err error) {
	frm := url.Values{
		"session_token":  {sessionToken},
		"delete_related": {strconv.FormatBool(deleteRelated)},
	}
	r := c.NewRequest("POST", "/1/sessions/delete", nil, frm)
	return r.DoJsonResponse()
}

// Validate a session token
func (c *AccountsClient) ValidateSession(sessionToken string) (response *http.Response, jdata Json, err error) {
	frm := url.Values{
		"session_token": {sessionToken},
	}
	r := c.NewRequest("POST", "/1/sessions/validate", nil, frm)
	return r.DoJsonResponse()
}

// Check username availability
func (c *AccountsClient) UsernameAvailable(username string) (response *http.Response, jdata Json, err error) {
	r := c.NewRequest("GET", "/1/usernames/"+username, nil, nil)
	return r.DoJsonResponse()
}

// Checks username availability using a query parameter
func (c *AccountsClient) YetAnotherUsernameAvailable(username string) (response *http.Response, jdata Json, err error) {
	r := c.NewRequest("GET", "/1/username-check?username="+username, nil, nil)
	return r.DoJsonResponse()
}

// Query users using the GET /1/users endpoint
// accountStatus is one or more of active,deactivated,purged as CSV
// filter is a key=val where key is one of username, email, family_name
// orderBy is one of user_id,username,email,family_name
type ListUserMods struct {
	Status   []string
	Filters  map[string]string
	Limit    int
	OrderBy  string
	Offset   int
	SortDesc bool
}

func (c *AccountsClient) ListUsers(mods ListUserMods) (response *http.Response, jdata Json, err error) {
	values := url.Values{}
	if mods.Status != nil {
		s := strings.Join(mods.Status, ",")
		values.Set("status", s)
	}
	if mods.Filters != nil {
		for key, val := range mods.Filters {
			values.Set(key, val)
		}
	}
	if mods.OrderBy != "" {
		values.Set("order_by", mods.OrderBy)
	}
	if mods.SortDesc {
		values.Set("sort_descending", "true")
	}
	if mods.Limit != 0 {
		values.Set("limit", strconv.Itoa(mods.Limit))
	}
	if mods.Offset != 0 {
		values.Set("offset", strconv.Itoa(mods.Offset))
	}
	r := c.NewRequest("GET", "/1/users?"+values.Encode(), nil, nil)
	return r.DoJsonResponse()
}
