// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// The netacuity package provides a low level query interface to the Netacuity API
package netacuity

import (
	"errors"
	"net/http"
	"net/url"
	"strings"

	"github.com/anki/sai-go-util/http/apiclient"
)

const (
	USAURL    = "http://usa.cloud.netacuity.com"
	EuropeURL = "http://europe.cloud.netacuity.com"
	AsiaURL   = "http://asia.cloud.netacuity.com"
	QueryPath = "/webservice/query"

	AllDB           DB = "all"
	GeoDB           DB = "3"
	EdgeDB          DB = "4"
	SICDB           DB = "5"
	DomainDB        DB = "6"
	ZipAreaTimeDB   DB = "7"
	ISPDB           DB = "8"
	HomeBizDB       DB = "9"
	ASNDB           DB = "10"
	LangDB          DB = "11"
	ProxyDB         DB = "12"
	IsAnISPDB       DB = "14"
	CompanyNameDB   DB = "15"
	DemographicDB   DB = "17"
	NAICSDB         DB = "18"
	CBSADB          DB = "19"
	MobileCarrierDB DB = "24"
	OrgDB           DB = "25"
)

type DB string

type Server struct {
	APIURL string
	APIKey string
}

type DBSet []DB

func (dbs DBSet) String() string {
	tmp := make([]string, len(dbs))
	for i, d := range dbs {
		tmp[i] = string(d)
	}
	return strings.Join(tmp, ",")
}

func NewDBSet(db ...DB) DBSet {
	result := make(DBSet, len(db))
	for i, d := range db {
		result[i] = d
	}
	return result
}

// QueryIP queries the NetAcuity API and returns the raw json response
func (s *Server) QueryIP(dbs DBSet, IPAddr string) (result []byte, err error) {
	client, err := apiclient.New("netacuity",
		apiclient.WithServerURL(s.APIURL),
		apiclient.WithBuildInfoUserAgent(),
		apiclient.WithDefaultRetry())

	if err != nil {
		return nil, err
	}

	v := url.Values{}
	v.Set("u", s.APIKey)
	v.Set("ip", IPAddr)
	v.Set("dbs", dbs.String())
	v.Set("json", "true")

	req, err := client.NewRequest("GET", QueryPath,
		apiclient.WithQueryValues(v))
	if err != nil {
		return nil, err
	}

	resp, err := client.Do(req)

	if err != nil {
		return nil, err
	}

	body, err := resp.Data()
	if err != nil {
		return nil, err
	}

	if resp.StatusCode != http.StatusOK {
		return nil, errors.New("Did not get OK response from server: " + string(body))
	}

	return body, nil
}
