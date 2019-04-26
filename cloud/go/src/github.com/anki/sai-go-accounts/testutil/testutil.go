// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// This should be imported into all unit tests
package testutil

import (
	"github.com/anki/sai-go-accounts/email"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/postmarkapp"
)

func init() {
	alog.ToStdout()
}

// CaptureEmails runs a function and captures all of the messages it generates
func CaptureEmails(f func() error) (msgs []*postmarkapp.Message, err error) {
	email.InitAnkiEmailer()
	defer email.InitAnkiEmailer()
	pmc := email.Emailer.PostmarkC.(*postmarkapp.Client)
	pmc.LastEmail = make(chan *postmarkapp.Message)
	pmc.LastBatchEmail = make(chan []*postmarkapp.Message)
	done := make(chan error)
	go func() {
		done <- f()
	}()
	for {
		select {
		case msg := <-pmc.LastEmail:
			msgs = append(msgs, msg)
		case batch := <-pmc.LastBatchEmail:
			msgs = append(msgs, batch...)
		case err = <-done:
			return msgs, err
		}
	}
}
