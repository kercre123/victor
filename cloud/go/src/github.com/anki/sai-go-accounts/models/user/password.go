// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// A hashed password holds (surprisingly) a hash of a password, along with enough
// meta data to be able to validate it at a later time.

package user

import (
	"database/sql/driver"
	"encoding/base64"
	"errors"
	"fmt"
	"io"
	"strconv"
	"strings"
	"unicode"

	"github.com/anki/sai-go-accounts/apierrors"
	"github.com/anki/sai-go-util/strutil"

	"golang.org/x/crypto/scrypt"
)

const (
	saltLength    = 16
	DefaultHasher = "scrypt"
)

var Hashers map[string]Hasher = map[string]Hasher{}

func RegisterHasher(name string, hasher Hasher) {
	Hashers[name] = hasher
}

func init() {
	RegisterHasher("scrypt", Scrypt{N: 16384, R: 8, P: 1, KeyLen: 32})
}

// HashedPasssword holds a hashed version of a pasword.
//
// The hash also includes sufficient metadata about the hash algorithm and parameters
// used to generate the hash to be able to validate it.
type HashedPassword string

// Validate determines if the supplied password matches the hashed password.
func (hp HashedPassword) Validate(password []byte) (bool, error) {
	pieces := strings.SplitN(string(hp), "$", 2)
	if len(pieces) != 2 {
		return false, errors.New("Didn't find separator in hashed password")
	}
	hashname, encoded := pieces[0], pieces[1]
	hasher, ok := Hashers[hashname]
	if !ok {
		return false, errors.New(fmt.Sprintf("Unknown hash algorithm %q", hashname))
	}
	return hasher.Validate(encoded, password)
}

func NewHashedPassword(hashName string, password []byte) (HashedPassword, error) {
	if h, ok := Hashers[hashName]; ok {
		result, err := h.Encode(password)
		if err != nil {
			return "", err
		}
		return HashedPassword(hashName + "$" + result), nil
	}
	return "", errors.New(fmt.Sprintf("Unknown hash algorithm %q", hashName))
}

// Value implements the db/driver.Valuer interface.
func (hp HashedPassword) Value() (driver.Value, error) {
	return string(hp), nil
}

// Scan implements the db.Scanner interface
func (hp *HashedPassword) Scan(src interface{}) error {
	switch v := src.(type) {
	case string:
		*hp = HashedPassword(v)
	case []byte:
		*hp = HashedPassword(v)
	default:
		return errors.New("Scan source was not a string")
	}
	return nil
}

func salt() (s []byte, err error) {
	s = make([]byte, saltLength)
	_, err = io.ReadFull(strutil.All, s)
	return s, err
}

func parseInts(buf []int, str []string) error {
	var err error
	for i, s := range str {
		buf[i], err = strconv.Atoi(s)
		if err != nil {
			return err
		}
	}
	return nil
}

type Hasher interface {
	Encode(password []byte) (encoded string, err error)
	Validate(encoded string, password []byte) (match bool, err error)
}

type Scrypt struct {
	N      int
	R      int
	P      int
	KeyLen int
}

func (s Scrypt) Encode(password []byte) (encoded string, err error) {
	salt, err := salt()
	if err != nil {
		return "", err
	}
	dk, err := scrypt.Key(password, salt, s.N, s.R, s.P, s.KeyLen)
	if err != nil {
		return "", err
	}
	return fmt.Sprintf("%d$%d$%d$%d$%s$%s", s.N, s.R, s.P, s.KeyLen, salt, base64.StdEncoding.EncodeToString(dk)), nil
}

func (s Scrypt) Validate(encoded string, password []byte) (match bool, err error) {
	var (
		numbers         []int = make([]int, 5)
		n, r, p, keyLen int
		salt, hash      string
	)
	pieces := strings.Split(encoded, "$")
	if len(pieces) != 6 {
		return false, errors.New("Misformatted hash metadata")
	}

	if err := parseInts(numbers, pieces[:4]); err != nil {
		return false, errors.New("Misformatted hash metadata")
	}
	n, r, p, keyLen = numbers[0], numbers[1], numbers[2], numbers[3]
	salt, hash = pieces[4], pieces[5]

	dk, err := scrypt.Key(password, []byte(salt), n, r, p, keyLen)
	if err != nil {
		return false, err
	}
	dkb64 := base64.StdEncoding.EncodeToString(dk)
	return dkb64 == hash, nil
}

// A more or less direct port of Ben's Python password_score function.
// TODO: replace this with something much better.
func PasswordScore(username, password string) (score int) {
	if strings.Contains(password, username) {
		return 0
	}
	if len(password) > 7 {
		score++
	}
	if len(password) > 11 {
		score++
	}
	var foundDigit, foundLower, foundUpper, foundPunct bool
	for _, chRune := range password {
		switch {
		case !foundDigit && unicode.IsDigit(chRune):
			foundDigit = true
			score++
		case !foundLower && unicode.IsLower(chRune):
			foundLower = true
		case !foundUpper && unicode.IsUpper(chRune):
			foundUpper = true
		case !foundPunct && unicode.IsPunct(chRune):
			foundPunct = true
			score++
		}
	}
	if foundLower && foundUpper {
		score++
	}
	return score
}

func PasswordIsStrong(username, password string) (bool, error) {
	if len(password) < 6 {
		return false, apierrors.PasswordTooShort
	}
	if strings.Contains(password, username) {
		return false, apierrors.PasswordContainsUsername
	}
	return PasswordScore(username, password) >= 3, nil
}
