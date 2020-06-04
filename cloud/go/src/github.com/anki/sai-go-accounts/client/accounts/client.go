package accounts

import (
	"net/http"
	"net/url"
	"strconv"

	api "github.com/anki/sai-go-util/http/apiclient"
)

type AccountsClient struct {
	*api.Client
}

func NewAccountsClient(prefix string, config ...api.Config) (*AccountsClient, error) {
	client, err := api.New(prefix, config...)
	if err != nil {
		return nil, err
	}
	return &AccountsClient{
		Client: client,
	}, nil
}

// ----------------------------------------------------------------------
// User API
// ----------------------------------------------------------------------

// Fetch a user, by userid.
func (c *AccountsClient) UserById(userId string) (*api.Response, error) {
	r, err := c.NewRequest("GET", "/1/users/"+userId)
	if err != nil {
		return nil, err
	}
	return c.Do(r)
}

// Update existing user
func (c *AccountsClient) UpdateUser(userId string, userInfo map[string]interface{}) (*api.Response, error) {
	r, err := c.NewRequest("PATCH", "/1/users/"+userId,
		api.WithJsonBody(userId))
	if err != nil {
		return nil, err
	}
	return c.Do(r)
}

// Delete (deactivate) a user account.
func (c *AccountsClient) DeleteUser(userId string) (*api.Response, error) {
	r, err := c.NewRequest("DELETE", "/1/users/"+userId)
	if err != nil {
		return nil, err
	}
	return c.Do(r)
}

// Reactivate a user account
func (c *AccountsClient) ReactivateUser(userId, userName string) (*api.Response, error) {
	r, err := c.NewRequest("POST", "/1/users/"+userId+"/reactivate",
		api.WithFormBody(url.Values{
			"username": []string{userName},
		}))
	if err != nil {
		return nil, err
	}
	return c.Do(r)
}

func (c *AccountsClient) ResendVerify(userId string) (*api.Response, error) {
	r, err := c.NewRequest("POST", "/1/users/"+userId+"/verify_email")
	if err != nil {
		return nil, err
	}
	return c.Do(r)
}

// ----------------------------------------------------------------------
// Session API
// ----------------------------------------------------------------------

// Login to a user account using a username & password.
func (c *AccountsClient) NewUserSession(userName, password string) (*api.Response, error) {
	r, err := c.NewRequest("POST", "/1/sessions",
		api.WithNoSession(),
		api.WithFormBody(url.Values{
			"username": {userName},
			"password": {password},
		}))
	if err != nil {
		return nil, err
	}
	return c.Do(r)
}

func (c *AccountsClient) NewUserIdSession(userId, password string) (*api.Response, error) {
	r, err := c.NewRequest("POST", "/1/sessions",
		api.WithNoSession(),
		api.WithFormBody(url.Values{
			"userid":   {userId},
			"password": {password},
		}))
	if err != nil {
		return nil, err
	}
	return c.Do(r)
}

// Reup a session using an existing session token
func (c *AccountsClient) ReupSession() (*api.Response, error) {
	r, err := c.NewRequest("POST", "/1/sessions/reup")
	if err != nil {
		return nil, err
	}
	return c.Do(r)
}

// Delete a session
func (c *AccountsClient) DeleteSession(sessionToken string, deleteRelated bool) (*api.Response, error) {
	r, err := c.NewRequest("POST", "/1/sessions/delete",
		api.WithFormBody(url.Values{
			"session_token":  {sessionToken},
			"delete_related": {strconv.FormatBool(deleteRelated)},
		}))
	if err != nil {
		return nil, err
	}
	return c.Do(r)
}

// Validate a session token
func (c *AccountsClient) ValidateSession(appkey, sessionToken string) (*api.Response, error) {
	r, err := c.NewRequest("POST", "/1/sessions/validate",
		api.WithFormBody(url.Values{
			"app_token":     {appkey},
			"session_token": {sessionToken},
		}))
	if err != nil {
		return nil, err
	}
	return c.Do(r)
}

type RelatedAccountsResult struct {
	Status  string   `json:"status"`
	UserIDs []string `json:"user_ids"`
}

// Fetch related accounts
func (c *AccountsClient) FetchRelatedAccounts(userId string) (*api.Response, *RelatedAccountsResult, error) {
	var result RelatedAccountsResult

	r, err := c.NewRequest("GET", "/1/users/"+userId+"/related")
	if err != nil {
		return nil, nil, err
	}
	resp, err := c.Do(r)
	if err != nil || resp.StatusCode != http.StatusOK {
		return resp, nil, err
	}

	if err = resp.UnmarshalJson(&result); err != nil {
		return resp, nil, err
	}
	return resp, &result, err
}
