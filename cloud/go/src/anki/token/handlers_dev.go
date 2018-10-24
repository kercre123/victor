// +build !shipping

package token

import (
	"anki/token/jwt"
	"clad/cloud"
	"fmt"
	"net/http"
)

func init() {
	devHandlers = func(s *http.ServeMux) {
		s.HandleFunc("/tokenauth", provisionHandler)
	}
}

func provisionHandler(w http.ResponseWriter, r *http.Request) {
	// See if we already have a JWT token and there's no reason for us to re-auth
	existing := jwt.GetToken()
	if existing != nil {
		// If the existing JWT token has an empty user ID, it's a dummy token we should
		// replace with a real one - if non-empty, we don't need to re-auth
		if existing.UserID() != "" {
			w.WriteHeader(http.StatusBadRequest)
			fmt.Fprint(w, "No auth necessary, robot already has token!")
			return
		}
	}

	// If we're here, we need to authorize with the provided session token
	if err := r.ParseForm(); err != nil {
		w.WriteHeader(http.StatusBadRequest)
		fmt.Fprint(w, "Error parsing form: ", err)
		return
	}
	tok := r.Form.Get("token")
	if tok == "" {
		w.WriteHeader(http.StatusBadRequest)
		fmt.Fprint(w, "No token value provided in request")
		return
	}

	authRequest := cloud.NewTokenRequestWithAuth(&cloud.AuthRequest{SessionToken: tok})
	if authResp, err := HandleRequest(authRequest); err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		fmt.Fprint(w, "Error attempting authorization: ", err)
		return
	} else if terr := authResp.GetAuth().Error; terr != cloud.TokenError_NoError {
		w.WriteHeader(http.StatusInternalServerError)
		fmt.Fprint(w, "Unexpected error response from token authorization: ", terr)
		return
	}
	fmt.Fprint(w, "Robot should now be authorized!")
}
