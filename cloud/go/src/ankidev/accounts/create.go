package accounts

import (
	"errors"
	"fmt"
	"net/http"

	"github.com/anki/sai-go-util/http/apiclient"
)

// DoCreate attempts to create an account with the given email and password
func DoCreate(email, password string) (apiclient.Json, error) {
	c, _, err := newClient()
	if err != nil {
		return nil, err
	}

	userInfo := map[string]interface{}{
		"password": password,
		"email":    email,
	}

	r, err := c.NewRequest("POST", "/1/users", apiclient.WithJsonBody(userInfo))
	if err != nil {
		return nil, err
	}
	resp, err := c.Do(r)
	if err != nil {
		return nil, err
	} else if resp.StatusCode != http.StatusOK {
		return nil, errors.New(fmt.Sprint("http status ", resp.StatusCode))
	}

	json, err := resp.Json()
	if err != nil {
		return nil, err
	}
	return json, nil
}
