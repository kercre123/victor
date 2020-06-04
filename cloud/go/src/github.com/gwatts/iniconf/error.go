// Copyright Splunk, Inc. 2014
//
// Author: Gareth Watts <gareth@splunk.com>

package iniconf

import (
	"errors"
	"fmt"
)

// ParseError codes.
const (
	ErrInvalidSection = iota
	ErrInvalidEntry
	ErrLineTooLong
	ErrNoSection
)

var (
	ErrDuplicateSection = errors.New("Duplicate section name")
	ErrInvalidEntryName = errors.New("Invalid name used for entry")
	ErrEntryNotFound    = errors.New("Entry not found in section")
	ErrSectionNotFound  = errors.New("Section not found")
	ErrReloadNoFilename = errors.New("Cannot reload; no filename specified")
	ErrSaveNoFilename   = errors.New("Cannot save; no filename specified")
	ErrEmptyEntryValue  = errors.New("Cannot assign empty string to an entry")
	ErrBadObserver      = errors.New("Attempted to register observer that does not implement an observer interface")
	ErrEmptyConfChain   = errors.New("ConfChain has no configuration files")
)

var (
	parseErrorMsgs = map[int]string{
		ErrInvalidSection: "Invalid section definition",
		ErrInvalidEntry:   "Failed to parse line with no \"=\" delimiter",
		ErrLineTooLong:    "Line too long",
		ErrNoSection:      "Value declared outside of section",
	}
)

// A ParseError is created if a syntax error is found
// while reading an ini file.
//
// Implements the error interface.
type ParseError struct {
	lineNum int
	code    int
}

// Return the line number of the source file the error refers to
func (p *ParseError) LineNum() int {
	return p.lineNum
}

func (p *ParseError) Code() int {
	return p.code
}

// Return the base message of the error, excluding the line number
// use the Error() method to get a message that includes the line number.
func (p ParseError) Msg() string {
	return parseErrorMsgs[p.code]
}

// Returns the error message including the line number.
func (pe ParseError) Error() string {
	return fmt.Sprintf("%s at line %d", pe.Msg(), pe.lineNum)
}

func newParseError(code int, lineNum int) *ParseError {
	return &ParseError{code: code, lineNum: lineNum}
}
