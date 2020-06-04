// Package jdocstest provides types and functions to create keys and documents
// for JDOCS testing. Can be used for unit testing and load testing.
package jdocstest

import (
	"encoding/json"
	"fmt"
	"strings"

	"github.com/anki/sai-jdocs/jdocs/db" // package jdocsdb
	"github.com/anki/sai-jdocs/proto/jdocspb"
)

type DocKey struct {
	Account        string
	Thing          string
	DocName        string
	DocType        string
	DocVersion     uint64
	FmtVersion     uint64
	NumDataStrings int // XXX: coarse way to control document size
}

// DocData is a simple document type that is easy to construct and verify.
type DocData struct {
	Account     string   `json:"Account"`
	Thing       string   `json:"Thing"`
	DocName     string   `json:"DocName"`
	DocVersion  uint64   `json:"DocVersion"`
	FmtVersion  uint64   `json:"FmtVersion"`
	DataStrings []string `json:"DataStrings"`
}

//////////////////////////////////////////////////////////////////////

func AccountName(aNum int) string {
	return fmt.Sprintf("a%d", aNum) // keep short to avoid ClientMetadata too long
}

func ThingName(tNum int) string {
	return fmt.Sprintf("t%d", tNum) // keep short to avoid ClientMetadata too long
}

// NewJdoc fills in all Jdoc fields based on the key fields.
func NewJdoc(key DocKey) (*jdocspb.Jdoc, error) {
	if key.DocVersion == 0 {
		return &jdocspb.Jdoc{
			DocVersion:     0,
			FmtVersion:     0,
			ClientMetadata: "",
			JsonDoc:        "",
		}, nil
	}

	// Don't include irrelevant values in the data!
	switch key.DocType {
	case jdocsdb.DocTypeAccount:
		key.Thing = ""
	case jdocsdb.DocTypeThing:
		key.Account = ""
	case jdocsdb.DocTypeAccountThing:
		// NOP
	default:
		return nil, fmt.Errorf("key.DocType=%s is invalid\n", key.DocType)
	}

	dd := DocData{
		Account:     key.Account,
		Thing:       key.Thing,
		DocName:     key.DocName,
		DocVersion:  key.DocVersion,
		FmtVersion:  key.FmtVersion,
		DataStrings: make([]string, key.NumDataStrings),
	}

	// "0000000000000000000000000000000000000000"
	// "1111111111111111111111111111111111111111"
	// ...
	for i, _ := range dd.DataStrings {
		dd.DataStrings[i] = strings.Repeat(fmt.Sprintf("%d", i%10), 40)
	}
	jsonDoc, err := json.Marshal(dd)
	if err != nil {
		return nil, err
	}

	return &jdocspb.Jdoc{
		DocVersion:     key.DocVersion,
		FmtVersion:     key.FmtVersion,
		ClientMetadata: key.Account + "_" + key.Thing + "_" + key.DocName,
		JsonDoc:        string(jsonDoc),
	}, nil
}
