// Copyright Splunk, Inc. 2014
//
// Author: Gareth Watts <gareth@splunk.com>

package iniconf

// A ConfChain reads sequentially from a list of IniConf configurations, returning
// a result from the first IniConf that has a value set for a requested setting.
//
// Useful if you want a .ini file on disk to override an in-memory set of defaults.
type ConfChain struct {
	confSet []*IniConf
}

// Create a new chain.
//
// IniConfs will be read from in preference from first to last specified here
// ie. the IniConf holding the default values to fallback to should be the
// last argument to this function.
func NewConfChain(confSet ...*IniConf) *ConfChain {
	return &ConfChain{confSet}
}

// AddConf adds an additional IniConf file to the chain at a lower priority than
// those already configured.
func (cc *ConfChain) AddConf(i *IniConf) {
	cc.confSet = append(cc.confSet, i)
}

func (cc *ConfChain) fetchEntry(sectionName, entryName string, fetcher valueFetcher) (interface{}, error) {
	if len(cc.confSet) == 0 {
		return "", ErrEmptyConfChain
	}
	for _, i := range cc.confSet {
		result, err := fetcher(i, sectionName, entryName)
		// allow any error other than entry not found to propogate
		if err != ErrEntryNotFound {
			return result, err
		}
	}

	return "", ErrEntryNotFound
}

type valueFetcher func(i *IniConf, sectionName, entryName string) (interface{}, error)

func stringFetcher(i *IniConf, sectionName, entryName string) (interface{}, error) {
	return i.EntryString(sectionName, entryName)
}

func intFetcher(i *IniConf, sectionName, entryName string) (interface{}, error) {
	return i.EntryInt(sectionName, entryName)
}

func boolFetcher(i *IniConf, sectionName, entryName string) (interface{}, error) {
	return i.EntryBool(sectionName, entryName)
}

func (cc *ConfChain) HasSection(sectionName string) bool {
	for _, i := range cc.confSet {
		if i.HasSection(sectionName) {
			return true
		}
	}
	return false
}

// EntryString fetches a string entry from a section from the first IniConf that returns a value.
func (cc *ConfChain) EntryString(sectionName, entryName string) (string, error) {
	result, err := cc.fetchEntry(sectionName, entryName, stringFetcher)
	return result.(string), err
}

// EntryStringP fetches a string entry from a section from the first IniConf that returns a value.
//
// Unlike EntryString(), It panics if no IniConfs contain the setting.
func (cc *ConfChain) EntryStringP(sectionName, entryName string) string {
	result, err := cc.fetchEntry(sectionName, entryName, stringFetcher)
	if err != nil {
		panic(err)
	}
	return result.(string)
}

// EntryInt fetches an integer entry from a section from the first IniConf that returns a value.
func (cc *ConfChain) EntryInt(sectionName, entryName string) (int64, error) {
	result, err := cc.fetchEntry(sectionName, entryName, intFetcher)
	if err != nil {
		return 0, err
	}
	return result.(int64), nil
}

// EntryIntP fetches an integer entry from a section from the first IniConf that returns a value.
//
// Unlike EntryInt(), It panics if no IniConfs contain the setting.
func (cc *ConfChain) EntryIntP(sectionName, entryName string) int64 {
	result, err := cc.fetchEntry(sectionName, entryName, intFetcher)
	if err != nil {
		panic(err)
	}
	return result.(int64)
}

// Fetch a boolean entry from a section from the first IniConf that returns a value.
func (cc *ConfChain) EntryBool(sectionName, entryName string) (bool, error) {
	result, err := cc.fetchEntry(sectionName, entryName, boolFetcher)
	if err != nil {
		return false, err
	}
	return result.(bool), nil
}

// EntryBoolP fetches a boolean entry from a section from the first IniConf that returns a value.
//
// Unlike EntryBool(), It panics if no IniConfs contain the setting.
func (cc *ConfChain) EntryBoolP(sectionName, entryName string) bool {
	result, err := cc.fetchEntry(sectionName, entryName, boolFetcher)
	if err != nil {
		panic(err)
	}
	return result.(bool)
}

// ReadSection maps an IniConf section into a struct
//
// Each field in the struct to be read from the section should be tagged
// with `iniconf:"keyname"`
//
// Only string, bool and int types are supported
//
// Nested structs are flattened into the parent's namespace.
func (cc *ConfChain) ReadSection(sectionName string, v interface{}) error {
	return readSection(cc, sectionName, v)
}
