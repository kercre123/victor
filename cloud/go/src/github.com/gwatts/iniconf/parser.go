// Copyright Splunk, Inc. 2014
//
// Author: Gareth Watts <gareth@splunk.com>

package iniconf

// handle low level lexing/parsing of .ini files
// this still needs to handle inline comments

import (
	"bufio"
	"bytes"
	"io"
	"strings"
	"unicode"
	"unicode/utf8"
)

const (
	entryTypeNone  = iota
	entryTypeBlank // blank line
	entryTypeComment
	entryTypeSection
	entryTypeKV
)

type parseValue struct {
	entryType   int
	commentLine []byte
	section     string
	key         string
	value       []byte
	rawLines    [][]byte // raw lines making up the value, including any trailing comment
	lineNum     int
}

func (pv *parseValue) addRawLine(line []byte) {
	rl := make([]byte, len(line))
	copy(rl, line)
	pv.rawLines = append(pv.rawLines, rl)
}

type parser struct {
	br      *bufio.Reader
	bufLine []byte
	lineNum int
}

func newParser(br *bufio.Reader) *parser {
	return &parser{br: br}
}

func (p *parser) nextLine() (line []byte, err error) {
	var isPrefix bool
	if p.bufLine != nil {
		return p.bufLine, nil
	}
	p.bufLine, isPrefix, err = p.br.ReadLine()
	if isPrefix {
		// line longer than 4KB
		return nil, newParseError(ErrLineTooLong, p.lineNum)
	}
	p.lineNum++
	return p.bufLine, err
}

func (p *parser) NextValue() (*parseValue, error) {
	result := new(parseValue)
	inValue := false
	for {
		rawLine, err := p.nextLine()
		// make copy
		rawLine = append([]byte{}, rawLine...)
		if err == io.EOF && inValue {
			// will return EOF on next read
			return result, nil
		} else if err != nil {
			return result, err
		}
		// strip trailing whitespace
		line := bytes.TrimRightFunc(rawLine, unicode.IsSpace)
		if inValue {
			if len(line) == 0 || !startsWithSpace(line) {
				// leave p.bufLine intact for next call
				return result, nil
			}
			result.value = append(result.value, '\n')
			result.value = append(result.value, bytes.TrimLeftFunc(line, unicode.IsSpace)...)
			result.addRawLine(rawLine)
			// we crossed the line, so mark it zero
			p.bufLine = nil
		} else if len(line) > 0 {
			result.lineNum = p.lineNum
			switch line[0] {
			case ';':
				result.entryType = entryTypeComment
				result.addRawLine(rawLine)
				p.bufLine = nil
				return result, nil
			case '[':
				if line[len(line)-1] != ']' {
					return nil, newParseError(ErrInvalidSection, p.lineNum)
				}
				result.entryType = entryTypeSection
				result.section = strings.TrimFunc(string(line[1:len(line)-1]), unicode.IsSpace)
				result.addRawLine(rawLine)
				p.bufLine = nil
				return result, nil
			default:
				kv := bytes.SplitN(line, []byte("="), 2)
				if len(kv) != 2 {
					return nil, newParseError(ErrInvalidEntry, p.lineNum)
				}
				result.entryType = entryTypeKV
				result.key = string(bytes.TrimRightFunc(kv[0], unicode.IsSpace))
				value := bytes.TrimLeftFunc(kv[1], unicode.IsSpace)
				result.value = append([]byte{}, value...)
				result.addRawLine(rawLine)
				// the next line may be a continuation of this value
				inValue = true
				p.bufLine = nil
			}
		} else {
			// empty line
			result.entryType = entryTypeBlank
			result.addRawLine([]byte(""))
			p.bufLine = nil
			return result, nil
		}
	}
}

func startsWithSpace(s []byte) bool {
	r, _ := utf8.DecodeRune(s)
	if r == utf8.RuneError {
		return false
	}
	return unicode.IsSpace(r)
}
