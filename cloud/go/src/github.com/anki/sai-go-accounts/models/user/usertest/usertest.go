// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package usertest

import (
	"github.com/anki/sai-go-accounts/models/user"
	"github.com/anki/sai-go-accounts/util"
)

// The Test interface allows either a testing.T or a testing.B to be
// passed in to the functions provided in this package.
type Test interface {
	Error(args ...interface{})
	Errorf(format string, args ...interface{})
	Fatal(args ...interface{})
	Fatalf(format string, args ...interface{})
}

func CreateModelUserFromUA(t Test, ua *user.UserAccount) *user.UserAccount {
	errs := user.CreateUserAccount(ua)
	if errs != nil {
		t.Fatalf("Failed to create user account: %s", errs)
	}
	return ua

}
func CreateNamedModelUser(t Test, username string, remoteId string) *user.UserAccount {
	ua := &user.UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &user.User{
			Status:    user.AccountActive,
			Username:  util.Pstring(username),
			GivenName: util.Pstring("given name"),
			Email:     util.Pstring("a@b"),
		}}
	return CreateModelUserFromUA(t, ua)
}

func CreateModelUser(t Test) *user.UserAccount {
	return CreateNamedModelUser(t, "user1", "")
}

func CreateModelUser2(t Test) *user.UserAccount {
	return CreateNamedModelUser(t, "user2", "")
}

func CreateUserWithState(t Test, status user.AccountStatus, username, email, familyName string) *user.UserAccount {
	ua := &user.UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &user.User{
			Status:     status,
			Username:   &username,
			FamilyName: &familyName,
			Email:      &email,
		},
	}
	return CreateModelUserFromUA(t, ua)
}
