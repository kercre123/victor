package accounts

import (
	"fmt"
	"net/http"

	"github.com/anki/sai-go-cli/config"
)

// DoLogin attempts to log in to the Accounts service with the given credentials. The returned
// session references the given config but NOT the other way around - call session.Save() to
// persist it to disk and make sure it remains associated with the returned configuration in
// the future. For temporary use, the values in the session (i.e. token) can be used independently
// of the config without saving it.
func DoLogin(envName, user, pass string) (*config.Session, *config.Config, error) {
	c, cfg, err := newClient(envName)
	resp, err := c.NewUserSession(user, pass)
	if err != nil {
		return nil, nil, err
	} else if resp.StatusCode != http.StatusOK {
		json, err := resp.Json()
		if err != nil || json == nil {
			return nil, nil, fmt.Errorf("http status %d: (no JSON body)", resp.StatusCode)
		}
		return nil, nil, fmt.Errorf("http status %d: (%q)", resp.StatusCode, json["message"])
	}

	jresp, err := resp.Json()
	if err != nil {
		return nil, nil, err
	}

	s, err := cfg.NewSession("default")
	if err != nil {
		return nil, nil, err
	}

	if userID, _ := jresp.FieldStr("user", "user_id"); userID != "" {
		s.UserID = userID
	}

	token, err := jresp.FieldStr("session", "session_token")
	if err != nil {
		return nil, nil, err
	}
	s.Token = token
	return s, cfg, nil
}
