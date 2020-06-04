/*
apiclient provides a base client type and utilities for building
clients for Anki's HTTP APIs. 

The base Client type handles Anki API authentication, and all other
Anki-specific headers, as well as utilities for building requests. 

Example:

	r, err := c.NewRequest("POST", "/1/sessions",
		api.WithFormBody(url.Values{
			"username": {userName},
			"password": {password},
		}))
	if err != nil {
		return nil, err
	}
	return c.Do(r)

*/
package apiclient
