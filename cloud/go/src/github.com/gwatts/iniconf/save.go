// Copyright Splunk, Inc. 2014
//
// Author: Gareth Watts <gareth@splunk.com>

package iniconf

import (
	"bufio"
	"bytes"
	"fmt"
	"io"
	"os"
)

type sectionBuf struct {
	lines     [][]byte
	blankSeen bool
	// track the last line number in a section block that holds an actual key/val
	lastKV          int
	lastLeadComment int
}

func newSectionBuf() sectionBuf {
	return sectionBuf{
		lastKV:          -1,
		lastLeadComment: -1,
		lines:           make([][]byte, 0),
	}
}

// add lines to the last section
func (s *sectionBuf) appendLines(entryType int, lines [][]byte) {
	s.lines = append(s.lines, lines...)
	switch entryType {
	case entryTypeBlank:
		s.blankSeen = true
	case entryTypeSection:
		fallthrough
	case entryTypeComment:
		if !s.blankSeen {
			s.lastLeadComment = len(s.lines) - len(lines)
		}
	case entryTypeKV:
		s.lastKV = len(s.lines) - 1
	}
}

type saver struct {
	i *IniConf
	// store list of section lines
	buf []sectionBuf

	// map section names to outbuf offsets (we're only interested in the last time
	// we see a section, in case of split sections)
	lastSectionOffsets map[string]int

	// keep a map of section->keys that have been seen in the file already
	seenKeys map[string]map[string]bool
}

func newSaver(i *IniConf) *saver {
	result := &saver{
		i:                  i,
		lastSectionOffsets: make(map[string]int),
		seenKeys:           make(map[string]map[string]bool),
	}
	result.addSection("") // first "section" is the header/preamble
	return result
}

func (s *saver) save() error {
	err := s.readFile()
	if err != nil {
		return err
	}

	s.mergeValues()

	outfile, err := os.OpenFile(s.i.filename, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0666)
	if err != nil {
		return err
	}
	defer outfile.Close()
	return s.doSave(outfile)
}

func (s *saver) String() string {
	buf := new(bytes.Buffer)
	s.mergeValues()
	if err := s.doSave(buf); err != nil {
		panic(fmt.Sprintf("Save error: %s", err))
	}
	return buf.String()
}

func (s *saver) lastSection() *sectionBuf {
	return &s.buf[len(s.buf)-1]
}

func (s *saver) addSection(name string) {
	s.buf = append(s.buf, newSectionBuf())
	s.lastSectionOffsets[name] = len(s.buf) - 1
}

func (s *saver) readFile() error {
	i := s.i
	// Open the existing file for read
	readfile, err := os.Open(i.filename)
	if os.IsNotExist(err) {
		return nil
	}
	if err != nil {
		return err
	}

	// parse the existing file and populate s.buf
	br := bufio.NewReader(readfile)
	parser := newParser(br)
	inHeader := true
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
			inHeader = false
			currentSection = i.section(parseEntry.section)
			if currentSection != nil {
				s.addSection(currentSection.idxName)
				s.lastSection().appendLines(
					entryTypeSection, parseEntry.rawLines[0:1])
				if s.seenKeys[currentSection.idxName] == nil {
					s.seenKeys[currentSection.idxName] = make(map[string]bool)
				}
			} // else the section is present on disk, but not in i (ie. deleted)

		case entryTypeKV:
			if currentSection != nil {
				// section isn't deleted
				newEntry := currentSection.entry(parseEntry.key)
				if newEntry != nil {
					// entry hasn't been deleted from i
					if string(parseEntry.value) != newEntry.value {
						s.lastSection().appendLines(
							entryTypeKV,
							formatKV(parseEntry.key, newEntry.value))

					} else {
						// leave original formatting if unchanged
						s.lastSection().appendLines(
							entryTypeKV, parseEntry.rawLines)
					}
					s.seenKeys[currentSection.idxName][i.idxName(parseEntry.key)] = true
				}
			}

		case entryTypeBlank:
			if inHeader && len(s.lastSection().lines) == 0 {
				// don't add blank lines to the beginning of the file
				break
			}
			if currentSection != nil || inHeader {
				// keep the comment/blank line, if in the file header, or in a non-deleted section
				s.lastSection().appendLines(entryTypeBlank, parseEntry.rawLines)
			}

		case entryTypeComment:
			if currentSection != nil || inHeader {
				// keep the comment/blank line, if in the file header, or in a
				// non-deleted section
				s.lastSection().appendLines(entryTypeComment, parseEntry.rawLines)
			}
		}
	}
	return nil
}

// Add any values that weren't in the on-disk file to the buf,
// in the order they were added via NewSetion()/SetEntry()
func (s *saver) mergeValues() {
	i := s.i
	for sel := i.sectionList.Front(); sel != nil; sel = sel.Next() {
		section := sel.Value.(*section)
		seen := s.seenKeys[section.idxName]
		if seen == nil {
			// append a section to the file
			s.addSection(section.idxName)
			if len(s.buf) > 2 { // 2 == preamble plus the one we just added
				// add a blank line to separate sections
				s.lastSection().appendLines(entryTypeSection, [][]byte{[]byte("")})
			}
			s.lastSection().appendLines(
				entryTypeSection,
				[][]byte{[]byte(fmt.Sprintf("[%s]", section.name))})
			for eel := section.entryList.Front(); eel != nil; eel = eel.Next() {
				e := eel.Value.(*entry)
				s.lastSection().appendLines(
					entryTypeKV, formatKV(e.name, e.value))
			}
		} else {
			// add to an existing section; find the last section block that
			// references that section
			bn := s.lastSectionOffsets[section.idxName]
			ln := s.buf[bn].lastKV
			if ln == -1 {
				// if the section had no kv lines in it, then insert at at the
				// beginning, after any initial comments
				ln = s.buf[bn].lastLeadComment
			}
			// find unreferenced keys
			newLines := make([][]byte, 0)
			for eel := section.entryList.Front(); eel != nil; eel = eel.Next() {
				e := eel.Value.(*entry)
				if !s.seenKeys[section.idxName][i.idxName(e.name)] {
					newLines = append(newLines, formatKV(e.name, e.value)...)
				}
			}
			// insert into section
			s.buf[bn].lines = append(s.buf[bn].lines[0:ln+1],
				append(newLines, s.buf[bn].lines[ln+1:]...)...)
		}
	}

}

// Do the actual save to file.
func (s *saver) doSave(outfile io.Writer) error {
	// Save to file

	for _, secblock := range s.buf {
		for _, line := range secblock.lines {
			outfile.Write(line)
			_, err := outfile.Write([]byte("\n"))
			if err != nil {
				return err
			}
		}
	}
	return nil
}
