package jdocs

// specs.go
//
// Specification for each document stored in a JDOCS deployment.

import (
	"fmt"

	"github.com/anki/sai-jdocs/jdocs/db" // package jdocsdb
)

// JdocSpec is a specification for a Jdoc. It is minimal and organized to be
// easy to define (eg JSON-format-friendly). DocType is determined by the prefix
// of TableName field.
type JdocSpec struct {
	DocName        string       `json:"DocName"`
	Description    string       `json:"Description"` // documentation only
	TableName      string       `json:"TableName"`
	HasGDPRData    bool         `json:"HasGDPRData"`    // some of the data in this document falls under GDPR regulations
	ExpectedMaxLen int          `json:"ExpectedMaxLen"` // Splunk warning for each JsonDoc write whose length exceeds this
	AllowCreate    []ClientType `json:"AllowCreate"`
	AllowRead      []ClientType `json:"AllowRead"`
	AllowUpdate    []ClientType `json:"AllowUpdate"`
	AllowDelete    []ClientType `json:"AllowDelete"`
}

type JdocSpecList struct {
	Specs []JdocSpec `json:"Specs"`
}

// intJdocSpec represents a JdocSpec in a more way that is more convenient for
// internal use ("int" == "internal"). For example, maps are used for fast and
// easy lookup of per-ClientType document permissions.
type intJdocSpec struct {
	DocName        string
	DocType        string
	TableName      string
	HasGDPRData    bool
	ExpectedMaxLen int
	AllowCreate    map[ClientType]bool
	AllowRead      map[ClientType]bool
	AllowUpdate    map[ClientType]bool
	AllowDelete    map[ClientType]bool
}

// intJdocSpecSet : Map docName --> intJdocSpec.
type intJdocSpecSet map[string]intJdocSpec

//////////////////////////////////////////////////////////////////////

// toIntJdocSpec expands a (more concise) JdocSpecSet into a (less concise)
// intJdocSpecSet. These contain equivalent information.
func (sl *JdocSpecList) toIntJdocSpecSet() (*intJdocSpecSet, error) {
	iSpecSet := make(intJdocSpecSet)

	for _, spec := range sl.Specs {
		iSpec := intJdocSpec{
			DocName:        spec.DocName,
			DocType:        "", // fill this in below
			TableName:      spec.TableName,
			HasGDPRData:    spec.HasGDPRData,
			ExpectedMaxLen: spec.ExpectedMaxLen,
			AllowCreate:    make(map[ClientType]bool),
			AllowRead:      make(map[ClientType]bool),
			AllowUpdate:    make(map[ClientType]bool),
			AllowDelete:    make(map[ClientType]bool),
		}

		// TODO: Make sure docName   has proper format and charcters?
		// TODO: Make sure tableName has proper format and charcters?

		iSpec.DocType = jdocsdb.TableDocType(spec.TableName)
		if iSpec.DocType == jdocsdb.DocTypeUnknown {
			return &intJdocSpecSet{}, fmt.Errorf("TableName=%s must start with a valid DocType indicator", spec.TableName)
		}

		// Populate AllowCRUD ClientType maps, from lists
		for _, ct := range spec.AllowCreate {
			iSpec.AllowCreate[ct] = true
		}
		for _, ct := range spec.AllowRead {
			iSpec.AllowRead[ct] = true
		}
		for _, ct := range spec.AllowUpdate {
			iSpec.AllowUpdate[ct] = true
		}
		for _, ct := range spec.AllowDelete {
			iSpec.AllowDelete[ct] = true
		}

		iSpec.AllowCreate[ClientTypeSuperuser] = true
		iSpec.AllowRead[ClientTypeSuperuser] = true
		iSpec.AllowUpdate[ClientTypeSuperuser] = true
		iSpec.AllowDelete[ClientTypeSuperuser] = true

		iSpecSet[spec.DocName] = iSpec
	}

	return &iSpecSet, nil
}
