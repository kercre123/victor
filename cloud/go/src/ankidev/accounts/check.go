package accounts

import (
	"errors"
)

// CheckUsername returns whether or not the given username is available
// (and an error if it could not be determined)
func CheckUsername(envName, username string) (bool, error) {
	c, _, err := newClient(envName)
	if err != nil {
		return false, err
	}

	r, err := c.NewRequest("GET", "/1/usernames/"+username)
	if err != nil {
		return false, err
	}
	if resp, err := c.Do(r); err != nil {
		return false, err
	} else if json, err := resp.Json(); err != nil {
		return false, err
	} else if val, err := json.Field("exists"); err != nil {
		// it appears that if an account IS available, an error is returned,
		// rather than simply having "exists" == false
		return true, nil
	} else if exists, ok := val.(bool); !ok {
		return false, errors.New("'exists' not a bool in username check")
	} else {
		return !exists, nil
	}
}
