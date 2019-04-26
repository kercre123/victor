// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// +build !nolibxmlsec1

package session

import (
	"encoding/base64"
	"net/http"
	"net/url"
	"strings"

	"github.com/anki/sai-go-accounts/apis"
	"github.com/anki/sai-go-accounts/models/session"
	"github.com/anki/sai-go-util/jsonutil"
	"github.com/anki/sai-go-util/saml"
)

func runSetupSAMLAPI(SAMLResponse, relayState string) (req *http.Request, record *jsonutil.JsonWriterRecorder) {
	params := url.Values{
		"SAMLResponse": []string{SAMLResponse},
		"RelayState":   []string{relayState},
	}
	return apitest.RunRequest("", nil, "POST", "/1/sessions/saml", nil, params)
}

func (s *SessionAPISuite) TestSAMLParseError() {
	// supply an invalid XML SAML doc
	t := s.T()

	defer func(adp *saml.SAMLIDP) { activeSAMLIDP = adp }(activeSAMLIDP)
	activeSAMLIDP = garethtest02IDP

	_, resp := runSetupSAMLAPI("invalid-xml", base64.StdEncoding.EncodeToString([]byte("http://example.com/")))
	if resp.ErrorResponse == nil || resp.ErrorResponse.(*jsonutil.JsonErrorResponse).Code != "invalid_saml_response" {
		t.Fatal("Incorrect error response", resp.ErrorResponse)
	}
}

func (s *SessionAPISuite) TestSAMLBadGroup() {
	// SAML checks pass, but user isn't a member of a valid group
	t := s.T()

	defer func(adp *saml.SAMLIDP) { activeSAMLIDP = adp }(activeSAMLIDP)
	activeSAMLIDP = garethtest02IDPNoExpire

	_, resp := runSetupSAMLAPI(bobbySAMLB64, base64.StdEncoding.EncodeToString([]byte("http://example.com/")))
	if resp.ErrorResponse == nil || resp.ErrorResponse.(*jsonutil.JsonErrorResponse).Code != "invalid_saml_group" {
		t.Fatal("Incorrect error response", resp.ErrorResponse)
	}
}

func (s *SessionAPISuite) TestSAMLOK() {
	// SAML checks pass; bobby is only a member of the Everyone group; explicitly allow access to that group
	t := s.T()

	defer func() { samlGroupMapping = []samlGroup{} }()
	defer func(adp *saml.SAMLIDP) { activeSAMLIDP = adp }(activeSAMLIDP)

	activeSAMLIDP = garethtest02IDPNoExpire
	AddSAMLGroup("Everyone", session.AdminScope)

	_, resp := runSetupSAMLAPI(bobbySAMLB64, base64.StdEncoding.EncodeToString([]byte("http://example.com/")))
	if resp.ErrorResponse != nil {
		t.Fatal("Unexpected error", resp.ErrorResponse)
	}

	if cookie := resp.HeaderMap.Get("Set-Cookie"); !strings.HasPrefix(cookie, "anki-auth") {
		t.Error("Cookie not set", cookie)
	}

	if location := resp.HeaderMap.Get("Location"); location != "http://example.com/" {
		t.Error("Location not set", location)
	}
}

func (s *SessionAPISuite) TestSAMLExpired() {
	t := s.T()

	defer func() { samlGroupMapping = []samlGroup{} }()
	defer func(adp *saml.SAMLIDP) { activeSAMLIDP = adp }(activeSAMLIDP)

	activeSAMLIDP = garethtest02IDP
	AddSAMLGroup("Everyone", session.AdminScope)

	_, resp := runSetupSAMLAPI(bobbySAMLB64, base64.StdEncoding.EncodeToString([]byte("http://example.com/")))
	if resp.ErrorResponse == nil || resp.ErrorResponse.(*jsonutil.JsonErrorResponse).Code != "invalid_saml_response" {
		t.Fatal("Incorrect error response", resp.ErrorResponse)
	}
}

func (s *SessionAPISuite) TestSAMLBadAudience() {
	t := s.T()

	defer func() { samlGroupMapping = []samlGroup{} }()
	defer func(adp *saml.SAMLIDP) { activeSAMLIDP = adp }(activeSAMLIDP)

	activeSAMLIDP = garethtest02IDPBadAudience
	AddSAMLGroup("Everyone", session.AdminScope)

	_, resp := runSetupSAMLAPI(bobbySAMLB64, base64.StdEncoding.EncodeToString([]byte("http://example.com/")))
	if resp.ErrorResponse == nil || resp.ErrorResponse.(*jsonutil.JsonErrorResponse).Code != "invalid_saml_response" {
		t.Fatal("Incorrect error response", resp.ErrorResponse)
	}
}
