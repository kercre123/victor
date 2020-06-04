// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package user

import (
	"testing"

	"github.com/anki/sai-go-accounts/apierrors"
	_ "github.com/anki/sai-go-accounts/testutil"
)

func TestHasher(t *testing.T) {
	password1 := []byte("some password")
	password2 := []byte("some wrong password")
	for hasherName, _ := range Hashers {
		encoded, err := NewHashedPassword(hasherName, password1)
		if err != nil {
			t.Errorf("Unexpected encode error for hasher=%q err=%v", hasherName, err)
			continue
		}
		// test matching password
		match, err := encoded.Validate(password1)
		if err != nil {
			t.Errorf("Unexpected validate error for hasher=%q err=%v", hasherName, err)
			continue
		}
		if !match {
			t.Errorf("Password did not verify for hasher=%q encoded=%q", hasherName, encoded)
		}
		// test bad password
		match, err = encoded.Validate(password2)
		if err != nil {
			t.Errorf("Unexpected validate error for hasher=%q err=%v", hasherName, err)
			continue
		}
		if match {
			t.Errorf("Password verified when it should of failed for hasher=%q encoded=%q", hasherName, encoded)
		}

	}
}

var scoreTests = []struct {
	password      string
	expectedScore int
}{
	{"aaaaaa", 0},
	{"AAAAAA", 0},
	{"AAAAAAAA", 1},
	{"AAAAAAAAAAAA", 2}, // 12
	{"123456", 1},
	{"AAAaaa", 1},
	{"1AAaaa", 2},
	{"1AAaa.", 3},
	{"1AAaa.", 3},
}

func TestPasswordScore(t *testing.T) {
	for _, test := range scoreTests {
		score := PasswordScore("username", test.password)
		if score != test.expectedScore {
			t.Errorf("password=%q score mismatch expected=%d actual=%d",
				test.password, test.expectedScore, score)
		}
	}
}

var strengthTests = []struct {
	username      string
	password      string
	isStrong      bool
	expectedError error
}{
	{"foobar", "foo", false, apierrors.PasswordTooShort},
	{"foobar", "foobar", false, apierrors.PasswordContainsUsername},
	{"foobar", "aaaaaa", false, nil},
	{"foobar", "1AAaa.", true, nil},
}

func TestPasswordIsStrong(t *testing.T) {
	for _, test := range strengthTests {
		isStrong, err := PasswordIsStrong(test.username, test.password)
		if test.expectedError != err {
			t.Errorf("username=%q password=%q expectederror=%v actual=%v",
				test.username, test.password, test.expectedError, err)
		}
		if test.isStrong != isStrong {
			t.Errorf("username=%q password=%q expectedstrong=%t actual=%t",
				test.username, test.password, test.isStrong, isStrong)
		}
	}
}
