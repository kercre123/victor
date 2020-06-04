// Copyright Splunk, Inc. 2014
//
// Author: Gareth Watts <gareth@splunk.com>

/*
 Read/Write settings from a .ini formatted configuration file.

 This package allows for .ini text files to be created, loaded, reloaded, saved and updated.
 It also allows for notifications to be sent to observers when sections
 or keys change either explictly via calls to the instance, or via a call to Reload()

 When saving/updating files it preserves the original order of entries that were in the file
 on disk at the point it's saved, along with any comments and spacing contained in it.


 The IniConf type reads and writes .ini files and provides access to their contents.

 The ChainConf type allows multiple .ini files (on disk or in memory) to be chained together,
 allowing for in-memory defaults to be overriden by an on-disk configuration file.
*/
package iniconf

// TODO:
// + support escape characters
// + support multiline values containing blank lines / leading/trailing whitespace

import (
	"bufio"
	"container/list"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
	"sync"
	"unicode"
)

// An IniConf holds configuration information read from a file in .ini format.
//
// ini files are made up of sections containing entry keys and values.
//
// All public methods of IniConf are safe to use from concurrent goroutines.
//
// This implementation supports line comments beginning with a semi colon
// and multi-line values where the second and subsequent lines for the value
// begin with a space.
//
// Keys and values may be any UTF8 strings.
//
// Comments inline with a value are not supported.
type IniConf struct {
	filename      string
	caseSensitive bool
	// provide quick access to sections by name
	sectionMap map[string]*list.Element
	// maintains section names in the order they were added
	sectionList *list.List
	// observe changes to any section
	globalObservers map[interface{}]interface{}
	// observe changes to individual sections
	sectionObservers  map[string]map[interface{}]interface{}
	mutex             sync.RWMutex
	saveMutex         sync.Mutex
	observersDisabled bool
}

// New creates a new unnamed .ini file.
//
// Specifying caseSensitive=true causes section and entry names to
// be cAsE sensitive (ie. section1 and Section1 map to two different sections)
//
// Use SaveToFilename() to save.
func New(caseSensitive bool) *IniConf {
	return &IniConf{
		sectionMap:       make(map[string]*list.Element),
		sectionList:      list.New(),
		caseSensitive:    caseSensitive,
		globalObservers:  make(map[interface{}]interface{}),
		sectionObservers: make(map[string]map[interface{}]interface{}),
	}
}

// NewFile creates a new .ini file with defined filename.
//
// Use Save() to write the contents to disk.
func NewFile(filename string, caseSensitive bool) *IniConf {
	result := New(caseSensitive)
	result.filename = filename
	return result
}

// LoadFile loads a .ini file from an existing file on disk.
///
// Once loaded it can be edited and saved by calling Save().
func LoadFile(filename string, caseSensitive bool) (*IniConf, error) {
	f, err := os.Open(filename)
	if err != nil {
		return nil, err
	}

	i := New(caseSensitive)
	i.filename = filename
	err = i.load(f)
	if err != nil {
		return nil, err
	}
	return i, nil
}

// LoadReader loads a .ini file from an io.Reader.
//
// Use SaveToFilename() to save.
func LoadReader(r io.Reader, caseSensitive bool) (*IniConf, error) {
	i := New(caseSensitive)
	err := i.load(r)
	if err != nil {
		return nil, err
	}
	return i, nil
}

// LoadEnviron loads a .ini file from environment variables.
//
// Each environment variable must in the format:
// prefix__section__key=value
func LoadEnviron(prefix string, caseSensitive bool) (*IniConf, error) {
	i := New(caseSensitive)
	i.loadEnviron(prefix)
	return i, nil
}

// LoadString load a .ini file from a string.
//
// Use SaveToFilename() to save.
func LoadString(s string, caseSensitive bool) (*IniConf, error) {
	return LoadReader(strings.NewReader(s), caseSensitive)
}

// Attempt to read an parse a .ini file.
func (i *IniConf) load(r io.Reader) error {
	i.mutex.Lock()
	defer i.mutex.Unlock()

	i.sectionMap = make(map[string]*list.Element)
	i.sectionList = list.New()
	br := bufio.NewReader(r)
	parser := newParser(br)

	var currentSection *section
	for {
		parseEntry, err := parser.NextValue()
		if err == io.EOF {
			break
		}
		if err != nil {
			return err
		}

		switch parseEntry.entryType {
		case entryTypeSection:
			currentSection = i.section(parseEntry.section)
			if currentSection == nil {
				currentSection, err = i.newSection(parseEntry.section)
				if err != nil {
					return err
				}
			}

		case entryTypeKV:
			if currentSection == nil {
				return newParseError(ErrNoSection, parseEntry.lineNum)
			}
			_, err := currentSection.setEntry(parseEntry.key, string(parseEntry.value))
			if err != nil {
				return err
			}
		}
	}
	return nil
}

func (i *IniConf) loadEnviron(prefix string) {
	i.mutex.Lock()
	defer i.mutex.Unlock()

	var err error
	prefix += "__"
	for _, keyval := range os.Environ() {
		if !strings.HasPrefix(keyval, prefix) {
			continue
		}
		kv := strings.SplitN(keyval[len(prefix):], "=", 2)
		sectionkey := strings.SplitN(kv[0], "__", 2)
		if len(sectionkey) != 2 {
			continue
		}
		section, key, value := strings.TrimSpace(sectionkey[0]), strings.TrimSpace(sectionkey[1]), strings.TrimSpace(kv[1])
		if section == "" || key == "" || value == "" {
			continue
		}
		s := i.section(section)
		if s == nil {
			s, err = i.newSection(section)
			if err != nil {
				// shouldn't happen; silently ignore
				continue
			}
		}
		// ignore errors
		s.setEntry(key, value)
	}
}

// Save the .ini file by updating any existing version on disk.
//
// This will attempt to preserve the order of the entries in the original file
// along with comment in sections that weren't deleted.
//
// The file must either have been initially loaded from file using LoadFile()
// or already been saved to a file using SaveToFilename().
func (i *IniConf) Save() error {
	if i.filename == "" {
		return ErrSaveNoFilename
	}
	// only one save can run concurrently
	i.saveMutex.Lock()
	defer i.saveMutex.Unlock()
	// we only need a read lock on the underyling data
	i.mutex.RLock()
	defer i.mutex.RUnlock()

	s := newSaver(i)
	return s.save()
}

// SaveToFilename save the IniConf to a new filename - Future calls to Save() will utilize
// the filename passed to this method.
func (i *IniConf) SaveToFilename(filename string) error {
	i.filename = filename
	return i.Save()
}

// String returns the IniConf file as a string.
func (i *IniConf) String() string {
	// we only need a read lock on the underyling data
	i.mutex.RLock()
	defer i.mutex.RUnlock()

	s := newSaver(i)
	return s.String()
}

// Get a lower case version of a name if the IniConf is set
// to have case insensitive section/key names.
func (i *IniConf) idxName(name string) string {
	if i.caseSensitive {
		return name
	}
	return strings.ToLower(name)
}

func (i *IniConf) newSection(name string) (*section, error) {
	idxName := i.idxName(name)
	if _, set := i.sectionMap[idxName]; set {
		return nil, ErrDuplicateSection
	}
	section := &section{
		iniConf:   i,
		name:      name,
		idxName:   idxName,
		entryMap:  make(map[string]*list.Element),
		entryList: list.New(),
	}
	el := i.sectionList.PushBack(section)
	i.sectionMap[idxName] = el
	return section, nil
}

// NewSection creates a new empty section.
// It returns ErrDuplicateSection if the section already exists.
//
// Note: If you just want to add a new entry to a section that doesn't yet exist
// then you can just call SetEntry() directly and the section will be auto-created.
func (i *IniConf) NewSection(name string) (err error) {
	var s *section
	defer func() {
		if s != nil {
			i.notifyNewSection(s)
		}
	}()
	i.mutex.Lock()
	defer i.mutex.Unlock()
	s, err = i.newSection(name)
	return err
}

func (i *IniConf) deleteSection(name string) error {
	idxName := i.idxName(name)
	if el, set := i.sectionMap[idxName]; set {
		delete(i.sectionMap, idxName)
		i.sectionList.Remove(el)
		return nil
	}
	return ErrSectionNotFound
}

// DeleteSection deletes a section and all of its entries.
//
// Will return ErrSectionNotFound if the section doesn't exist.
func (i *IniConf) DeleteSection(name string) error {
	var s *section
	defer func() {
		if s != nil {
			i.notifyDeleteSection(s)
		}
	}()
	i.mutex.Lock()
	defer i.mutex.Unlock()
	s = i.section(name) // will be nil if not found
	return i.deleteSection(name)
}

// Reload the configuration file by re-reading the file from disk.
// The .ini file must either have been created using LoadFile() or have
// been saved using SaveToFilename() for a reload to succeed.
//
// Any unsaved changes will be lost.
func (i *IniConf) Reload() error {
	// Keep track of changes; don't notify observers until changes are
	// complete and the mutex has been released
	newSections := make([]*section, 0)
	delSections := make([]*section, 0)
	entryChanges := make(map[*section][]entryChange, 0)

	// notify observers after mutex is released
	defer func() {
		for _, s := range delSections {
			i.notifyDeleteSection(s)
		}
		for _, s := range newSections {
			i.notifyNewSection(s)
		}
		for s, chlist := range entryChanges {
			i.notifyEntriesChanged(s, chlist)
		}
	}()

	i.mutex.Lock()
	defer i.mutex.Unlock()

	if i.filename == "" {
		return ErrReloadNoFilename
	}
	// attempt to load the file into a new instance
	newConf, err := LoadFile(i.filename, i.caseSensitive)
	if err != nil {
		return err
	}

	// scan through newConf, update i and build up a list of changes
	// for notification

	// Remove sections deleted from the conf file
	for sidxname, sel := range i.sectionMap {
		nsel := newConf.sectionMap[sidxname]
		if nsel == nil {
			// deleted; remove section from i
			s := sel.Value.(*section)
			i.deleteSection(s.name)
			delSections = append(delSections, s)
			for _, ename := range s.entryNames() {
				entry := s.entry(ename)
				entryChanges[s] = append(entryChanges[s], entryChange{
					s.name, ename, entry.value, ""})
			}
			continue
		}

		// Find updates to values in existing sections
		s, ns := sel.Value.(*section), nsel.Value.(*section)
		for _, ename := range s.entryNames() {
			entry := s.entry(ename)
			newEntry := ns.entry(ename)
			if newEntry == nil {
				// deleted
				s.deleteEntry(ename)
				entryChanges[s] = append(entryChanges[s], entryChange{
					s.name, ename, entry.value, ""})
			} else {
				if entry.value != newEntry.value {
					// changed
					entryChanges[s] = append(entryChanges[s], entryChange{
						s.name, ename, entry.value, newEntry.value})
					entry.value = newEntry.value
				}
			}
			// delete from newConf so only new items are left
			ns.deleteEntry(ename)
		}

		// Any remaining entries in this newConf section  are new
		for _, ename := range ns.entryNames() {
			newEntry := ns.entry(ename)
			s.setEntry(newEntry.name, newEntry.value)
			entryChanges[s] = append(entryChanges[s], entryChange{
				s.name, ename, "", newEntry.value})
		}
		// remove section from newConf now that we've scanned it
		newConf.deleteSection(s.name)
	}

	// Track new sections added to the conf file
	// only new sections are left in newConf at this point
	for name, el := range newConf.sectionMap {
		// repurpose newConf's new section as our own and insert
		ns := el.Value.(*section)
		ns.iniConf = i
		newEl := i.sectionList.PushBack(ns)
		i.sectionMap[name] = newEl
		newSections = append(newSections, ns)
		for _, ename := range ns.entryNames() {
			newEntry := ns.entry(ename)
			entryChanges[ns] = append(entryChanges[ns], entryChange{
				ns.name, ename, "", newEntry.value})
		}

	}

	return nil
}

// Sections returns a list of defined section names.
func (i *IniConf) Sections() []string {
	i.mutex.RLock()
	defer i.mutex.RUnlock()
	result := make([]string, i.sectionList.Len())
	for el, i := i.sectionList.Front(), 0; el != nil; el = el.Next() {
		result[i] = el.Value.(*section).name
		i++
	}
	return result
}

// HasSection returns true if the section is defined in the configuration
// even if the section contains no entries.
func (i *IniConf) HasSection(sectionName string) bool {
	i.mutex.RLock()
	defer i.mutex.RUnlock()
	idxName := i.idxName(sectionName)
	_, result := i.sectionMap[idxName]
	return result
}

func (i *IniConf) section(name string) *section {
	idxName := i.idxName(name)
	el := i.sectionMap[idxName]
	if el == nil {
		return nil
	}
	return el.Value.(*section)
}

// EntryString fetches an entry from a section as a string.
//
// Returns a ErrEntryNotFound error if the entry isn't in the section.
func (i *IniConf) EntryString(sectionName, entryName string) (string, error) {
	i.mutex.RLock()
	defer i.mutex.RUnlock()
	s := i.section(sectionName)
	if s == nil {
		return "", ErrEntryNotFound
	}
	entry := s.entry(entryName)
	if entry == nil {
		return "", ErrEntryNotFound
	}
	return entry.value, nil
}

// EntryInt fetches an entry from the section as integer.
//
// Returns a ErrEntryNotFound error if the entry isn't in the section.
func (i *IniConf) EntryInt(sectionName, entryName string) (int64, error) {
	estr, err := i.EntryString(sectionName, entryName)
	if err != nil {
		return 0, err
	}

	return strconv.ParseInt(estr, 0, 64)
}

// EntryBool  an entry from the section as a boolean.
//
// Returns a ErrEntryNotFound error if the entry isn't in the section.
func (i *IniConf) EntryBool(sectionName, entryName string) (bool, error) {
	estr, err := i.EntryString(sectionName, entryName)
	if err != nil {
		return false, err
	}

	return strconv.ParseBool(estr)
}

// DeleteEntry delets an entry from a section.
//
// Will return a ErrEntryNotFound error if the entry doesn't exist.
func (i *IniConf) DeleteEntry(sectionName, entryName string) (err error) {
	var (
		s        *section
		orgValue string
	)
	defer func() {
		// notify observers of change
		if err == nil {
			i.notifyEntryChanged(s, entryChange{
				s.name, entryName, orgValue, ""})
		}
	}()
	i.mutex.Lock()
	defer i.mutex.Unlock()

	s = i.section(sectionName)
	if s == nil {
		return ErrEntryNotFound
	}

	orgEntry := s.entry(entryName)
	if orgEntry == nil {
		return ErrEntryNotFound
	} else {
		orgValue = orgEntry.value
	}

	s.deleteEntry(entryName)
	return nil
}

// SetEntry creates or updates an entry in a section.
//
// Non-string values will be converted to a string using the fmt package.
//
// If a converted value, stripped of leading/trailing whitespace results in
// a string of length zero, then an ErrEmptyEntryValue error will be returned.
func (i *IniConf) SetEntry(sectionName, entryName string, value interface{}) (err error) {
	var (
		s                  *section
		newSection         bool
		orgValue, newValue string
	)
	defer func() {
		// notify observers of change
		if err == nil {
			if newSection {
				i.notifyNewSection(s)
			}

			i.notifyEntryChanged(s, entryChange{
				s.name, entryName, orgValue, newValue})
		}
	}()
	i.mutex.Lock()
	defer i.mutex.Unlock()

	s = i.section(sectionName)
	if s == nil {
		s, err = i.newSection(sectionName)
		if err != nil {
			return err
		}
		// ensure newSection notification is triggered
		newSection = true
	}

	orgEntry := s.entry(entryName)
	if orgEntry != nil {
		orgValue = orgEntry.value
	}
	newValue, err = s.setEntry(entryName, value)
	return err
}

// EntryNames returns a list of the entry names defined in a section.
//
// Returns nil if the section does not exist.
func (i *IniConf) EntryNames(sectionName string) (result []string) {
	i.mutex.RLock()
	defer i.mutex.RUnlock()
	s := i.section(sectionName)
	if s == nil {
		return nil
	}

	return s.entryNames()
}

// ReadSection maps an IniConf section into a struct
//
// Each field in the struct to be read from the section should be tagged
// with `iniconf:"keyname"`
//
// Only string, bool and int types are supported
//
// Nested structs are flattened into the parent's namespace.
func (i *IniConf) ReadSection(sectionName string, v interface{}) error {
	return readSection(i, sectionName, v)
}

// UpdateSection takes a struct tagged with `iniconf:"keyname"` fields
// and loads those fields into the named section.
//
// Nested structs are flattened into the parent's namespace.
//
// Existing fields in the second that are not defined in the struct are
// left intact.
func (i *IniConf) UpdateSection(sectionName string, v interface{}) error {
	return loadSection(i, sectionName, v)
}

// A section represents one named group of settings in the configuration file.
type section struct {
	iniConf   *IniConf
	name      string
	idxName   string
	entryMap  map[string]*list.Element
	entryList *list.List
}

// An individual setting in the config file.
type entry struct {
	name  string
	value string
}

func (s *section) entry(name string) *entry {
	idxName := s.iniConf.idxName(name)
	el := s.entryMap[idxName]
	if el == nil {
		return nil
	}
	return el.Value.(*entry)
}

func (s *section) newEntry(name string) *entry {
	idxName := s.iniConf.idxName(name)
	entry := &entry{
		name: name,
	}
	el := s.entryList.PushBack(entry)
	s.entryMap[idxName] = el
	return entry
}

func (s *section) deleteEntry(name string) bool {
	idxName := s.iniConf.idxName(name)
	el := s.entryMap[idxName]
	if el == nil {
		return false
	}
	s.entryList.Remove(el)
	delete(s.entryMap, idxName)
	return true
}

func (s *section) setEntry(name string, value interface{}) (string, error) {
	var ok bool
	if name, ok = validateName(name); !ok {
		return "", ErrInvalidEntryName
	}
	strValue := strings.TrimSpace(fmt.Sprint(value))
	if len(strValue) == 0 {
		return "", ErrEmptyEntryValue
	}
	entry := s.entry(name)
	if entry == nil {
		entry = s.newEntry(name)
	}
	entry.value = strValue

	return strValue, nil
}

func (s *section) entryNames() (result []string) {
	for el := s.entryList.Front(); el != nil; el = el.Next() {
		entry := el.Value.(*entry)
		result = append(result, entry.name)
	}
	return result
}

// trim whitespace from a name and return true if ok
// currently names must be at least 1 character long.
func validateName(name string) (string, bool) {
	name = strings.TrimFunc(name, unicode.IsSpace)
	if len(name) < 1 {
		return name, false
	}
	return name, true
}

// convert a possibly multiline string into something compatible
// with the ini format.
// this removes blank lines and adds 4 spaces to the beginning of
// subsequent lines.
func formatKV(key, value string) [][]byte {
	result := make([][]byte, 1)
	value = strings.Replace(value, "\r", "\n", -1)
	lines := strings.Split(value, "\n")
	result[0] = append(result[0], []byte(key)...)
	result[0] = append(result[0], []byte(" = ")...)
	result[0] = append(result[0], []byte(strings.TrimSpace(lines[0]))...)
	if len(lines) > 1 {
		for _, line := range lines[1:] {
			line = strings.TrimSpace(line)
			if len(line) == 0 {
				continue
			}
			result = append(result, []byte("    "+line))
		}
	}
	return result
}
