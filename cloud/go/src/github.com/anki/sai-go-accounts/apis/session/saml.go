// +build !nolibxmlsec1

package session

// Implement SAML/OKTA related APIs

import (
	"encoding/base64"
	"errors"
	"net/http"
	"strings"
	"time"

	"github.com/anki/sai-go-accounts/apierrors"
	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-accounts/validate"
	"github.com/anki/sai-go-util"
	"github.com/anki/sai-go-util/http/apirouter"
	"github.com/anki/sai-go-util/http/ratelimit"
	"github.com/anki/sai-go-util/jsonutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/saml"
)

const (
	// Identity Provider metadata provided by Okta for each app we have defined
	// There's one app for each accounts server deployment.
	localhostIDPMeta = `
<?xml version="1.0" encoding="UTF-8"?><md:EntityDescriptor xmlns:md="urn:oasis:names:tc:SAML:2.0:metadata" entityID="http://www.okta.com/ky8kq60mAHAPUKOQVWRP"><md:IDPSSODescriptor WantAuthnRequestsSigned="true" protocolSupportEnumeration="urn:oasis:names:tc:SAML:2.0:protocol"><md:KeyDescriptor use="signing"><ds:KeyInfo xmlns:ds="http://www.w3.org/2000/09/xmldsig#"><ds:X509Data><ds:X509Certificate>MIICkzCCAfygAwIBAgIGAUYi2jRmMA0GCSqGSIb3DQEBBQUAMIGMMQswCQYDVQQGEwJVUzETMBEG
A1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5jaXNjbzENMAsGA1UECgwET2t0YTEU
MBIGA1UECwwLU1NPUHJvdmlkZXIxDTALBgNVBAMMBGFua2kxHDAaBgkqhkiG9w0BCQEWDWluZm9A
b2t0YS5jb20wHhcNMTQwNTIyMDczMzI0WhcNNDQwNTIyMDczNDI0WjCBjDELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xDTALBgNVBAoMBE9r
dGExFDASBgNVBAsMC1NTT1Byb3ZpZGVyMQ0wCwYDVQQDDARhbmtpMRwwGgYJKoZIhvcNAQkBFg1p
bmZvQG9rdGEuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCOK8yb3rnNFNlzNz0u5RjX
nEdOyeKVeYyidV9moQXeODVrB9HCbd4tuz9fRYfZIwz5LOtzMVz+yFwtlWPwLOv2R3g0i8XTQ+EH
IAWFQtOZ5XRT9kAiH2tE2u2CuuD+hzHgVjGFTtwHzDcCzd1/L7mOoIp2z5uQdoD9qc6KdiId7QID
AQABMA0GCSqGSIb3DQEBBQUAA4GBAFCpdIZpyHeD91o9tqkXeXbhB0kIcfW963tA/wSeKUOJaRF9
iy2z4n+wqG+9Bic5t9PrklMUZ5e5J7gigonGX22RtD5arLEM0CiqE1ByephcRhNwt84LRStHrMUa
5kSzsQI7P+RojS1WmrL3Pp28sHlB9+TLpXWPJ4VMw/aazzve</ds:X509Certificate></ds:X509Data></ds:KeyInfo></md:KeyDescriptor><md:NameIDFormat>urn:oasis:names:tc:SAML:1.1:nameid-format:emailAddress</md:NameIDFormat><md:NameIDFormat>urn:oasis:names:tc:SAML:1.1:nameid-format:unspecified</md:NameIDFormat><md:SingleSignOnService Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST" Location="https://anki.okta.com/app/template_saml_2_0/ky8kq60mAHAPUKOQVWRP/sso/saml"/><md:SingleSignOnService Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-Redirect" Location="https://anki.okta.com/app/template_saml_2_0/ky8kq60mAHAPUKOQVWRP/sso/saml"/></md:IDPSSODescriptor></md:EntityDescriptor>
`

	garethIDPMeta = `
<?xml version="1.0" encoding="UTF-8"?><md:EntityDescriptor xmlns:md="urn:oasis:names:tc:SAML:2.0:metadata" entityID="http://www.okta.com/kz4a1rlmYIBAUKJHBXFS"><md:IDPSSODescriptor WantAuthnRequestsSigned="true" protocolSupportEnumeration="urn:oasis:names:tc:SAML:2.0:protocol"><md:KeyDescriptor use="signing"><ds:KeyInfo xmlns:ds="http://www.w3.org/2000/09/xmldsig#"><ds:X509Data><ds:X509Certificate>MIICkzCCAfygAwIBAgIGAUYi2jRmMA0GCSqGSIb3DQEBBQUAMIGMMQswCQYDVQQGEwJVUzETMBEG
A1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5jaXNjbzENMAsGA1UECgwET2t0YTEU
MBIGA1UECwwLU1NPUHJvdmlkZXIxDTALBgNVBAMMBGFua2kxHDAaBgkqhkiG9w0BCQEWDWluZm9A
b2t0YS5jb20wHhcNMTQwNTIyMDczMzI0WhcNNDQwNTIyMDczNDI0WjCBjDELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xDTALBgNVBAoMBE9r
dGExFDASBgNVBAsMC1NTT1Byb3ZpZGVyMQ0wCwYDVQQDDARhbmtpMRwwGgYJKoZIhvcNAQkBFg1p
bmZvQG9rdGEuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCOK8yb3rnNFNlzNz0u5RjX
nEdOyeKVeYyidV9moQXeODVrB9HCbd4tuz9fRYfZIwz5LOtzMVz+yFwtlWPwLOv2R3g0i8XTQ+EH
IAWFQtOZ5XRT9kAiH2tE2u2CuuD+hzHgVjGFTtwHzDcCzd1/L7mOoIp2z5uQdoD9qc6KdiId7QID
AQABMA0GCSqGSIb3DQEBBQUAA4GBAFCpdIZpyHeD91o9tqkXeXbhB0kIcfW963tA/wSeKUOJaRF9
iy2z4n+wqG+9Bic5t9PrklMUZ5e5J7gigonGX22RtD5arLEM0CiqE1ByephcRhNwt84LRStHrMUa
5kSzsQI7P+RojS1WmrL3Pp28sHlB9+TLpXWPJ4VMw/aazzve</ds:X509Certificate></ds:X509Data></ds:KeyInfo></md:KeyDescriptor><md:NameIDFormat>urn:oasis:names:tc:SAML:1.1:nameid-format:emailAddress</md:NameIDFormat><md:NameIDFormat>urn:oasis:names:tc:SAML:1.1:nameid-format:unspecified</md:NameIDFormat><md:SingleSignOnService Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST" Location="https://anki.okta.com/app/template_saml_2_0/kz4a1rlmYIBAUKJHBXFS/sso/saml"/><md:SingleSignOnService Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-Redirect" Location="https://anki.okta.com/app/template_saml_2_0/kz4a1rlmYIBAUKJHBXFS/sso/saml"/></md:IDPSSODescriptor></md:EntityDescriptor>`

	devIDPMeta = `
<?xml version="1.0" encoding="UTF-8"?><md:EntityDescriptor xmlns:md="urn:oasis:names:tc:SAML:2.0:metadata" entityID="http://www.okta.com/kzqvxyafJDZJGLEXJTOD"><md:IDPSSODescriptor WantAuthnRequestsSigned="true" protocolSupportEnumeration="urn:oasis:names:tc:SAML:2.0:protocol"><md:KeyDescriptor use="signing"><ds:KeyInfo xmlns:ds="http://www.w3.org/2000/09/xmldsig#"><ds:X509Data><ds:X509Certificate>MIICkzCCAfygAwIBAgIGAUYi2jRmMA0GCSqGSIb3DQEBBQUAMIGMMQswCQYDVQQGEwJVUzETMBEG
A1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5jaXNjbzENMAsGA1UECgwET2t0YTEU
MBIGA1UECwwLU1NPUHJvdmlkZXIxDTALBgNVBAMMBGFua2kxHDAaBgkqhkiG9w0BCQEWDWluZm9A
b2t0YS5jb20wHhcNMTQwNTIyMDczMzI0WhcNNDQwNTIyMDczNDI0WjCBjDELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xDTALBgNVBAoMBE9r
dGExFDASBgNVBAsMC1NTT1Byb3ZpZGVyMQ0wCwYDVQQDDARhbmtpMRwwGgYJKoZIhvcNAQkBFg1p
bmZvQG9rdGEuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCOK8yb3rnNFNlzNz0u5RjX
nEdOyeKVeYyidV9moQXeODVrB9HCbd4tuz9fRYfZIwz5LOtzMVz+yFwtlWPwLOv2R3g0i8XTQ+EH
IAWFQtOZ5XRT9kAiH2tE2u2CuuD+hzHgVjGFTtwHzDcCzd1/L7mOoIp2z5uQdoD9qc6KdiId7QID
AQABMA0GCSqGSIb3DQEBBQUAA4GBAFCpdIZpyHeD91o9tqkXeXbhB0kIcfW963tA/wSeKUOJaRF9
iy2z4n+wqG+9Bic5t9PrklMUZ5e5J7gigonGX22RtD5arLEM0CiqE1ByephcRhNwt84LRStHrMUa
5kSzsQI7P+RojS1WmrL3Pp28sHlB9+TLpXWPJ4VMw/aazzve</ds:X509Certificate></ds:X509Data></ds:KeyInfo></md:KeyDescriptor><md:NameIDFormat>urn:oasis:names:tc:SAML:1.1:nameid-format:emailAddress</md:NameIDFormat><md:NameIDFormat>urn:oasis:names:tc:SAML:1.1:nameid-format:unspecified</md:NameIDFormat><md:SingleSignOnService Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST" Location="https://anki.okta.com/app/template_saml_2_0/kzqvxyafJDZJGLEXJTOD/sso/saml"/><md:SingleSignOnService Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-Redirect" Location="https://anki.okta.com/app/template_saml_2_0/kzqvxyafJDZJGLEXJTOD/sso/saml"/></md:IDPSSODescriptor></md:EntityDescriptor>`

	betaIDPMeta = `
<?xml version="1.0" encoding="UTF-8"?><md:EntityDescriptor xmlns:md="urn:oasis:names:tc:SAML:2.0:metadata" entityID="http://www.okta.com/kzqvni6lYLVSAKESGALR"><md:IDPSSODescriptor WantAuthnRequestsSigned="true" protocolSupportEnumeration="urn:oasis:names:tc:SAML:2.0:protocol"><md:KeyDescriptor use="signing"><ds:KeyInfo xmlns:ds="http://www.w3.org/2000/09/xmldsig#"><ds:X509Data><ds:X509Certificate>MIICkzCCAfygAwIBAgIGAUYi2jRmMA0GCSqGSIb3DQEBBQUAMIGMMQswCQYDVQQGEwJVUzETMBEG
A1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5jaXNjbzENMAsGA1UECgwET2t0YTEU
MBIGA1UECwwLU1NPUHJvdmlkZXIxDTALBgNVBAMMBGFua2kxHDAaBgkqhkiG9w0BCQEWDWluZm9A
b2t0YS5jb20wHhcNMTQwNTIyMDczMzI0WhcNNDQwNTIyMDczNDI0WjCBjDELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xDTALBgNVBAoMBE9r
dGExFDASBgNVBAsMC1NTT1Byb3ZpZGVyMQ0wCwYDVQQDDARhbmtpMRwwGgYJKoZIhvcNAQkBFg1p
bmZvQG9rdGEuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCOK8yb3rnNFNlzNz0u5RjX
nEdOyeKVeYyidV9moQXeODVrB9HCbd4tuz9fRYfZIwz5LOtzMVz+yFwtlWPwLOv2R3g0i8XTQ+EH
IAWFQtOZ5XRT9kAiH2tE2u2CuuD+hzHgVjGFTtwHzDcCzd1/L7mOoIp2z5uQdoD9qc6KdiId7QID
AQABMA0GCSqGSIb3DQEBBQUAA4GBAFCpdIZpyHeD91o9tqkXeXbhB0kIcfW963tA/wSeKUOJaRF9
iy2z4n+wqG+9Bic5t9PrklMUZ5e5J7gigonGX22RtD5arLEM0CiqE1ByephcRhNwt84LRStHrMUa
5kSzsQI7P+RojS1WmrL3Pp28sHlB9+TLpXWPJ4VMw/aazzve</ds:X509Certificate></ds:X509Data></ds:KeyInfo></md:KeyDescriptor><md:NameIDFormat>urn:oasis:names:tc:SAML:1.1:nameid-format:emailAddress</md:NameIDFormat><md:NameIDFormat>urn:oasis:names:tc:SAML:1.1:nameid-format:unspecified</md:NameIDFormat><md:SingleSignOnService Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST" Location="https://anki.okta.com/app/template_saml_2_0/kzqvni6lYLVSAKESGALR/sso/saml"/><md:SingleSignOnService Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-Redirect" Location="https://anki.okta.com/app/template_saml_2_0/kzqvni6lYLVSAKESGALR/sso/saml"/></md:IDPSSODescriptor></md:EntityDescriptor>`

	prodIDPMeta = `
<?xml version="1.0" encoding="UTF-8"?><md:EntityDescriptor xmlns:md="urn:oasis:names:tc:SAML:2.0:metadata" entityID="http://www.okta.com/kzqvyl0xEGDXCWJSUJET"><md:IDPSSODescriptor WantAuthnRequestsSigned="true" protocolSupportEnumeration="urn:oasis:names:tc:SAML:2.0:protocol"><md:KeyDescriptor use="signing"><ds:KeyInfo xmlns:ds="http://www.w3.org/2000/09/xmldsig#"><ds:X509Data><ds:X509Certificate>MIICkzCCAfygAwIBAgIGAUYi2jRmMA0GCSqGSIb3DQEBBQUAMIGMMQswCQYDVQQGEwJVUzETMBEG
A1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5jaXNjbzENMAsGA1UECgwET2t0YTEU
MBIGA1UECwwLU1NPUHJvdmlkZXIxDTALBgNVBAMMBGFua2kxHDAaBgkqhkiG9w0BCQEWDWluZm9A
b2t0YS5jb20wHhcNMTQwNTIyMDczMzI0WhcNNDQwNTIyMDczNDI0WjCBjDELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xDTALBgNVBAoMBE9r
dGExFDASBgNVBAsMC1NTT1Byb3ZpZGVyMQ0wCwYDVQQDDARhbmtpMRwwGgYJKoZIhvcNAQkBFg1p
bmZvQG9rdGEuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCOK8yb3rnNFNlzNz0u5RjX
nEdOyeKVeYyidV9moQXeODVrB9HCbd4tuz9fRYfZIwz5LOtzMVz+yFwtlWPwLOv2R3g0i8XTQ+EH
IAWFQtOZ5XRT9kAiH2tE2u2CuuD+hzHgVjGFTtwHzDcCzd1/L7mOoIp2z5uQdoD9qc6KdiId7QID
AQABMA0GCSqGSIb3DQEBBQUAA4GBAFCpdIZpyHeD91o9tqkXeXbhB0kIcfW963tA/wSeKUOJaRF9
iy2z4n+wqG+9Bic5t9PrklMUZ5e5J7gigonGX22RtD5arLEM0CiqE1ByephcRhNwt84LRStHrMUa
5kSzsQI7P+RojS1WmrL3Pp28sHlB9+TLpXWPJ4VMw/aazzve</ds:X509Certificate></ds:X509Data></ds:KeyInfo></md:KeyDescriptor><md:NameIDFormat>urn:oasis:names:tc:SAML:1.1:nameid-format:emailAddress</md:NameIDFormat><md:NameIDFormat>urn:oasis:names:tc:SAML:1.1:nameid-format:unspecified</md:NameIDFormat><md:SingleSignOnService Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST" Location="https://anki.okta.com/app/template_saml_2_0/kzqvyl0xEGDXCWJSUJET/sso/saml"/><md:SingleSignOnService Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-Redirect" Location="https://anki.okta.com/app/template_saml_2_0/kzqvyl0xEGDXCWJSUJET/sso/saml"/></md:IDPSSODescriptor></md:EntityDescriptor>`
)

var (
	samlIDPs = map[string]*saml.SAMLIDP{
		"localhost": util.Must2(saml.NewSAMLIDP(localhostIDPMeta, "sai-accounts-localhost")).(*saml.SAMLIDP),
		"gareth":    util.Must2(saml.NewSAMLIDP(garethIDPMeta, "sai-accounts-gareth")).(*saml.SAMLIDP),
		"dev":       util.Must2(saml.NewSAMLIDP(devIDPMeta, "sai-accounts-dev")).(*saml.SAMLIDP),
		"beta":      util.Must2(saml.NewSAMLIDP(betaIDPMeta, "sai-accounts-beta")).(*saml.SAMLIDP),
		"prod":      util.Must2(saml.NewSAMLIDP(prodIDPMeta, "sai-accounts-prod")).(*saml.SAMLIDP),
	}

	activeSAMLIDP *saml.SAMLIDP
)

func SetActiveSAMLIDP(name string) error {
	if idp, ok := samlIDPs[name]; ok {
		activeSAMLIDP = idp
		return nil
	}
	return errors.New("Invalid IDP name")
}

func IDPNames() (names []string) {
	for k, _ := range samlIDPs {
		names = append(names, k)
	}
	return names
}

// Register the NewSAMLSession endpoint.
func init() {
	handler := jsonutil.JsonHandlerFunc(NewSAMLSession)
	RateLimiter = ratelimit.NewLimiter(loginLimiter{}, handler, LoginLimitRate, LoginLimitCapacity, time.Minute, nil)
	defineHandler(apirouter.Handler{
		Method: "GET",
		Path:   "/1/sessions/saml",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.MaybeSession,
			SessionScopeMask: session.AnyScope,
			NoAppKey:         true,
			Handler:          RateLimiter,
		}})
}

// The client's web browser is redirected here to do a SAML negotiated login
// This GET request does some basic checks and then redirect's to the SAML Identify Provider (Okta)
// which will ultimately have their browser do a POST back to this endpoint with a signed XML document
// containing their identity information.
func NewSAMLSession(w jsonutil.JsonWriter, r *http.Request) {
	returnTo := r.FormValue("return_to")

	if returnTo == "" {
		w.JsonError(r, apierrors.NewFieldValidationError("Missing fields"))
		return
	}
	url, err := activeSAMLIDP.GetRequestURL(returnTo)
	if err != nil {
		alog.Error{"action": "new_saml_session", "status": "idp_url_failed", "error": err}.LogR(r)
		w.JsonServerError(r)
		return
	}
	alog.Info{"action": "new_saml_session", "status": "redirect_to_idp", "url": url}.LogR(r)
	http.Redirect(w, r, url, 302)
}

// Register the SetupSAMLSession endpoint
func init() {
	handler := jsonutil.JsonHandlerFunc(SetupSAMLSession)
	RateLimiter = ratelimit.NewLimiter(loginLimiter{}, handler, LoginLimitRate, LoginLimitCapacity, time.Minute, nil)
	defineHandler(apirouter.Handler{
		Method: "POST",
		Path:   "/1/sessions/saml",
		Handler: &validate.ValidationHandler{
			SessionStatus:    validate.MaybeSession,
			SessionScopeMask: session.AnyScope,
			NoAppKey:         true,
			Handler:          RateLimiter,
		}})
}

// SetupSAMLSession processes this POST data the browser relays from the SAML IDP
// and sets up an admin session if it checks out before redirecting the user back to the
// originating application.
func SetupSAMLSession(w jsonutil.JsonWriter, r *http.Request) {
	xmlB64Response := r.PostFormValue("SAMLResponse")
	relayState := r.PostFormValue("RelayState")

	if xmlB64Response != "" {
		// Check that the SAML response is both parseable and signed by the trusted IDP.
		response, err := activeSAMLIDP.ParseResponse(xmlB64Response)
		if err != nil {
			alog.Error{"action": "new_saml_session", "status": "saml_parse_failure", "error": err}.LogR(r)
			w.JsonError(r, apierrors.InvaldSAML)
			return
		}

		if err := activeSAMLIDP.ValidateResponse(response); err != nil {
			alog.Error{"action": "new_saml_session", "status": "saml_validate_failure", "error": err}.LogR(r)
			w.JsonError(r, apierrors.InvaldSAML)
			return
		}

		// Find the highest privilege group the user is a member of.
		var assignScope session.Scope = 0
		var matchGroup string
		for _, gs := range samlGroupMapping {
			if response.IsGroupMember(gs.groupName) {
				assignScope = gs.scope
				matchGroup = gs.groupName
				break
			}
		}
		if assignScope == 0 {
			alog.Warn{"action": "new_saml_session", "status": "denied_not_group_member", "groups": strings.Join(response.Groups(), ",")}.LogR(r)
			w.JsonError(r, apierrors.InvaldSAMLGroup)
			return
		}

		session, err := session.NewAdminSession(session.AdminScope, response.SubjectName)
		if err != nil {
			w.JsonServerError(r)
			alog.Error{"action": "new_session_api", "status": "admin_session_create_failed", "error": err}.LogR(r)
			return
		}

		cn, cd := validate.AuthCookieName(r)
		http.SetCookie(w, &http.Cookie{Name: cn, Value: session.Session.Id, Path: "/", HttpOnly: true, Domain: cd})

		// retrieve redirect url
		targetUrl, _ := base64.StdEncoding.DecodeString(relayState)

		alog.Info{"action": "new_saml_session", "status": "ok", "user": response.SubjectName, "scope": assignScope.String(), "group": matchGroup, "target": targetUrl}.LogR(r)
		http.Redirect(w, r, string(targetUrl), 302)
		return

	} else {
		w.JsonError(r, apierrors.NewFieldValidationError("Missing fields"))
		return
	}
}
