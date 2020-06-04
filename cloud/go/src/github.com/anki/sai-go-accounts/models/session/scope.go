// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package session

import (
	"errors"
	"fmt"
	"strings"
)

type ScopeMask uint64

type Scope uint64

const (
	NoSessionScope Scope = 1
	RootScope      Scope = 2
	AdminScope     Scope = 4
	SupportScope   Scope = 8
	ServiceScope   Scope = 16 // used by ankival to validate sessions
	CronScope      Scope = 32 // used by cron caller
	UserScope      Scope = 64
)

const (
	AnyScope                   ScopeMask = (1 << 64) - 1 // including NoSessionScope
	AdminOrSupportScope        ScopeMask = ScopeMask(AdminScope | SupportScope)
	AdminOrCronScope           ScopeMask = ScopeMask(AdminScope | CronScope)
	AdminSupportOrServiceScope ScopeMask = ScopeMask(AdminScope | SupportScope | ServiceScope)
	ValidSessionScope          ScopeMask = AnyScope &^ ScopeMask(NoSessionScope)
)

var (
	ScopeNames = map[Scope]string{
		NoSessionScope: "nosession",
		RootScope:      "root",
		AdminScope:     "admin",
		SupportScope:   "support",
		ServiceScope:   "service",
		CronScope:      "cron",
		UserScope:      "user",
	}
	ScopeInverse = map[string]Scope{}
)

func init() {
	for scope, name := range ScopeNames {
		ScopeInverse[name] = scope
	}
}

// NewScopeMask combines several scopes into a several mask.
func NewScopeMask(scope ...Scope) (sm ScopeMask) {
	for _, s := range scope {
		sm |= ScopeMask(s)
	}
	return sm
}

// ParseScopeMask accepts a string scope mask and returns
// a ScopeMask type.
//
// The string must be comma separated.  eg. "user,support".
func ParseScopeMask(scopeMask string) (sm ScopeMask, err error) {
	for _, name := range strings.Split(scopeMask, ",") {
		if s, ok := ScopeInverse[name]; ok {
			sm |= ScopeMask(s)
		} else {
			return 0, fmt.Errorf("Invalid scope %q", name)
		}
	}
	return sm, nil
}

// Contains determines whether a given scope is present in a scope mask.
func (sm ScopeMask) Contains(scope Scope) bool {
	return uint(sm)&uint(scope) != 0
}

func (sm ScopeMask) String() string {
	names := []string{}
	for i := uint(0); i < 32; i++ {
		scopeval := ScopeMask(1) << i
		if sm&scopeval != 0 {
			if st := Scope(scopeval).String(); st != "" {
				names = append(names, st)
			}
		}
	}
	if len(names) > 0 {
		return strings.Join(names, ",")
	}
	return ""
}

func (s Scope) String() string {
	return ScopeNames[s]
}

func (s Scope) MarshalJSON() ([]byte, error) {
	result := append([]byte{'"'}, []byte(s.String())...)
	result = append(result, '"')
	return result, nil
}

func (s *Scope) UnmarshalJSON(data []byte) error {
	if len(data) < 3 || data[0] != '"' || data[len(data)-1] != '"' {
		return errors.New("Invalid data for scope")
	}
	if v, ok := ScopeInverse[string(data[1:len(data)-1])]; ok {
		*s = v
		return nil
	}
	return errors.New(fmt.Sprintf("Unknown scope name %q", string(data)))
}
