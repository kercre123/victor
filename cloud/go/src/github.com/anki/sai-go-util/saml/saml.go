// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// The saml package provides some convenience types for dealing with SAML assertions
package saml

import (
	"encoding/base64"
	"encoding/xml"
	"errors"
	"fmt"
	"time"

	"github.com/mattbaird/gosaml"
)

type SSOBinding struct {
	Binding  string `xml:"Binding,attr"`
	Location string `xml:"Location,attr"`
}

type SAMLIDPSSOMetadata struct {
	SignCert    string       `xml:"IDPSSODescriptor>KeyDescriptor>KeyInfo>X509Data>X509Certificate"` // public certificate
	EntityID    string       `xml:"entityID,attr"`
	NameFormats []string     `xml:"IDPSSODescriptor>NameIDFormat"`
	SSOBindings []SSOBinding `xml:"IDPSSODescriptor>SingleSignOnService"`
}

func (m *SAMLIDPSSOMetadata) SignPEM() string {
	return "-----BEGIN CERTIFICATE-----\n" + m.SignCert + "\n-----END CERTIFICATE-----\n"
}

func LoadSAMLIDPSSOMetadata(metadataXml string) (SAMLIDPSSOMetadata, error) {
	var result SAMLIDPSSOMetadata
	return result, xml.Unmarshal([]byte(metadataXml), &result)
}

func (s *SAMLIDPSSOMetadata) SSOByBinding(binding string) string {
	for _, b := range s.SSOBindings {
		if b.Binding == binding {
			return b.Location
		}
	}
	return ""
}

func (s *SAMLIDPSSOMetadata) PostLocation() string {
	return s.SSOByBinding("urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST")
}

func (s *SAMLIDPSSOMetadata) RedirectLocation() string {
	return s.SSOByBinding("urn:oasis:names:tc:SAML:2.0:bindings:HTTP-Redirect")
}

type SAMLAttribute struct {
	Name   string   `xml:"Name,attr"`
	Values []string `xml:"AttributeValue"`
}

type SAMLConditions struct {
	Audience     string `xml:"AudienceRestriction>Audience"`
	NotOnOrAfter string `xml:"NotOnOrAfter,attr"`
}

type SAMLResponse struct {
	RawXML      string
	SubjectName string          `xml:"Assertion>Subject>NameID"`
	Conditions  SAMLConditions  `xml:"Assertion>Conditions"`
	Attributes  []SAMLAttribute `xml:"Assertion>AttributeStatement>Attribute"`
}

func (s *SAMLResponse) Groups() []string {
	for _, attr := range s.Attributes {
		if attr.Name == "groups" {
			return attr.Values
		}
	}
	return nil
}

func (s *SAMLResponse) IsGroupMember(group string) bool {
	for _, gn := range s.Groups() {
		if gn == group {
			return true
		}
	}
	return false
}

// IsExpired returns true if the NotOnOrAfter field indicates
// the response is no longer valid based on the current time.
func (s *SAMLResponse) IsExpired() (bool, error) {
	// 2014-10-21T21:47:52.994Z
	exp, err := time.Parse(time.RFC3339, s.Conditions.NotOnOrAfter)
	if err != nil {
		return true, err
	}
	return exp.Before(time.Now()), nil
}

// The SAMLIDP type holds some information parsed from the metadata xml document
// provided by an identify provider.
//
// This information can be used to generate new identiy requests and validate responses.
type SAMLIDP struct {
	Metadata         SAMLIDPSSOMetadata
	AppSettings      *saml.AppSettings
	AccountSettings  *saml.AccountSettings
	NoExpiredCheck   bool
	ExpectedAudience string
}

func NewSAMLIDP(metadataXml, expectedAudience string) (*SAMLIDP, error) {
	m, err := LoadSAMLIDPSSOMetadata(metadataXml)
	if err != nil {
		return nil, err
	}
	return &SAMLIDP{
		Metadata:         m,
		AppSettings:      saml.NewAppSettings("", m.EntityID),
		AccountSettings:  saml.NewAccountSettings("", m.PostLocation()),
		ExpectedAudience: expectedAudience,
	}, nil
}

func (s *SAMLIDP) GetRequestURL(relayState string) (string, error) {
	authRequest := saml.NewAuthorizationRequest(*s.AppSettings, *s.AccountSettings)
	url, err := authRequest.GetRequestUrl()
	if err != nil {
		return "", err
	}
	url += "&RelayState=" + base64.StdEncoding.EncodeToString([]byte(relayState))
	return url, nil
}

// ParseResponse parses a SAML response and validates its signature using Okta's
// public certificate.
func (s *SAMLIDP) ParseResponse(xmlDataB64 string) (*SAMLResponse, error) {
	var response SAMLResponse

	xmlData, err := base64.StdEncoding.DecodeString(xmlDataB64)
	if err != nil {
		return nil, err
	}
	if err := xml.Unmarshal([]byte(xmlData), &response); err != nil {
		return nil, err
	}
	response.RawXML = string(xmlData)
	return &response, nil
}

// ValidateResponse checks that a SAML response has the correctly cryptographic
// signature (as determined by the certificate supplied by the IDP), is not expired
// and has the correct audience supplied.
func (s *SAMLIDP) ValidateResponse(r *SAMLResponse) error {
	if err := s.ValidateSignature(r.RawXML); err != nil {
		return err
	}
	// Prevent replay of expired responses
	if !s.NoExpiredCheck {
		isExpired, err := r.IsExpired()
		if err != nil {
			return err
		}
		if isExpired {
			return errors.New("Response assertion is expired")
		}
	}
	// Make sure a SAML assertion for another endpoint isn't replayed against this one
	if s.ExpectedAudience != r.Conditions.Audience {
		return fmt.Errorf("Audience mismatch - Expected %q, received %q", s.ExpectedAudience, r.Conditions.Audience)
	}
	return nil
}
