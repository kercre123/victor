// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// Package date provides a Date type to complement the standard Time type
// that can be easily (de)serialized to from a database and json/xml
package date

import (
	"database/sql/driver"
	"errors"

	"time"
)

const (
	// The string format used by String() and with database serialization
	Format = "2006-01-02"
)

var (
	// Zero holds an unitialized Date value.  It's IsZero method will return true.
	Zero = Date{}
)

// Date holds a year-month-day tuple and supports being serialized
// into a database string field using the standard sql/driver Valuer interface.
//
// It can hold dates from 0000-01-01 to 9999-12-31.
type Date struct {
	// stored as 16 bit year, 8 bit month, 8 bit day
	v uint32
}

// New creates a new Date.
func New(year, month, day int) (date Date, err error) {
	if year < 0 || year > 9999 || month < 1 || month > 12 || day < 1 || day > 31 {
		return date, errors.New("Invalid date")
	}
	return Date{uint32(year)<<16 | uint32(month)<<8 | uint32(day)}, nil
}

// MustNew creates a new date and panics if an out-of-range date is supplied.
func MustNew(year, month, day int) (date Date) {
	date, err := New(year, month, day)
	if err != nil {
		panic(err.Error())
	}
	return date
}

// Today returns today's date according to the current timezone.
func Today() Date {
	d, _ := FromTime(time.Now())
	return d
}

// Parse parses a date using the time package parser.
func Parse(layout, value string) (date Date, err error) {
	t, err := time.Parse(layout, value)
	if err != nil {
		return date, err
	}
	year, month, day := t.Date()
	return New(year, int(month), day)
}

// MustParse parses a date using the time package parser and panics on error.
func MustParse(layout, value string) (date Date) {
	date, err := Parse(layout, value)
	if err != nil {
		panic(err.Error())
	}
	return date
}

//FromTime extracts the date companent of a time.Time instance according
// to the timezone associated with that instance.
func FromTime(t time.Time) (Date, error) {
	year, month, day := t.Date()
	return New(year, int(month), day)
}

// Before returns true if date u occurs before u
func (d Date) Before(u Date) bool {
	return d.v < u.v
}

// After returns true if date u occurs after d
func (d Date) After(u Date) bool {
	return d.v > u.v
}

// Equal returns true if date u and d have the same value.
func (d Date) Equal(u Date) bool {
	return d.v == u.v
}

// Date returns the year, month and day components of the date.
func (d Date) Date() (year, month, day int) {
	year = int(d.v >> 16)
	month = int((d.v >> 8) & 0xff)
	day = int(d.v & 0xff)
	return year, month, day
}

// AddDate returns the date corresponding to adding the given number of years, months, and days to d.
// For example, AddDate(-1, 2, 3) applied to January 1, 2011 returns March 4, 2010.
//
// AddDate normalizes its result in the same way that Date does, so, for example, adding one
// month to October 31 yields December 1, the normalized form for November 31.
func (d Date) AddDate(year, month, day int) Date {
	if d.IsZero() {
		return d
	}

	result, _ := FromTime(d.Time(0, 0, 0, 0, time.UTC).AddDate(year, month, day))
	return result
}

// String returns a text version of the date in YYYY-MM-DD format.
// It will return an empty string if the date reprsents the zero (uninitialied) value.
func (d Date) String() string {
	if d.IsZero() {
		return ""
	}
	year, month, day := d.Date()
	t := time.Date(year, time.Month(month), day, 0, 0, 0, 0, time.UTC)
	return t.Format(Format)
}

// IsZero returns true if the instance is uninitialized
func (d Date) IsZero() bool {
	return d.v == 0
}

// Time returns a time.Time within the date.
func (d Date) Time(hour, min, sec, nsec int, loc *time.Location) time.Time {
	year, month, day := d.Date()
	return time.Date(year, time.Month(month), day, hour, min, sec, nsec, loc)
}

// sql driver interface implementation

// Value coverts the date to a string suitable for storage in the database.
func (d Date) Value() (driver.Value, error) {
	return driver.Value(d.String()), nil
}

// Scan converts a string returned from the database to a Date instance.
// An empty string maps to the zero value of a Date.
func (d *Date) Scan(src interface{}) error {
	s := string(src.([]byte))
	if s == "" {
		// Empty string should result in a zero Date
		*d = Zero
		return nil
	}

	u, err := Parse(Format, string(src.([]byte)))
	if err == nil {
		*d = u
	}
	return err
}

// encoding/TextMarshaler interface
func (d Date) MarshalText() (text []byte, err error) {
	return []byte(d.String()), nil
}

// encoding/TextUnmarshaler interface
func (d *Date) UnmarshalText(text []byte) error {
	return d.Scan(text)
}
