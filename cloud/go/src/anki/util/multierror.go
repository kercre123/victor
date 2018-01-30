package util

import (
	multierr "github.com/hashicorp/go-multierror"
)

type Errors struct {
	err error
}

func (e *Errors) Append(err error) {
	if err == nil {
		return
	}
	e.err = multierr.Append(e.err, err)
}

func (e *Errors) AppendMulti(errs ...error) {
	for _, err := range errs {
		e.Append(err)
	}
}

func (e *Errors) Error() error {
	return e.err
}
