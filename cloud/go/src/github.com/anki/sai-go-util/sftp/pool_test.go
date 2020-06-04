// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package sftp

import (
	"errors"
	"fmt"
	"io"
	"testing"
	"time"
)

func TestPoolDial(t *testing.T) {
	fakeSsh := setupSsh(nil)
	fakeSftp := setupSftp(fakeSsh, nil)
	p := NewClientPool(time.Minute, time.Minute)

	id := ClientID{
		Hostname: "testhost",
		Username: "auser",
		Password: "apassword",
	}

	c, err := p.Get(id)

	if err != nil {
		t.Fatal("Unexpected error from Dial", err)
	}

	if c == nil {
		t.Fatal("No client returned")
	}
	_ = fakeSftp

	if host := fakeSsh.recvAddr; host != "testhost" {
		t.Errorf("bad host expected=\"testhost\" actual=%q", host)
	}

	if user := fakeSsh.recvConfig.User; user != "auser" {
		t.Errorf("bad user expected=\"auser\" actual=%q", user)
	}

	auth := fakeSsh.recvConfig.Auth
	if len(auth) == 0 {
		t.Fatal("No auth methods")
	}

	// AuthMethod is a private interface in the sftp package.. not sure how it can be tested
	/*
		//cb, ok := auth[0].(func() (string, error))
		cb, ok := (auth[0]).(func() ([]ssh.Signer, error))
		if !ok {
			t.Fatal("Invalid auth type %t", auth[0])
		}
		if pw, _ := cb(); pw != "apassword" {
			t.Errorf("Incorrect password expected=\"apassword\" actual=%q", pw)
		}
	*/

	if c.cid == "" {
		t.Error("No cid set")
	}

	if c.fingerprint != fakeKey1Fingerprint {
		t.Errorf("Invalid fingerprint expected=%q actual=%q", fakeKey1Fingerprint, c.fingerprint)
	}
}

func TestPoolCache(t *testing.T) {
	// check that:
	// a second get results in a new client
	// releasing a client and doing another fetch results in re-use
	// releasing a client and fetching another id results in a new client

	fakeSsh := setupSsh(nil)
	fakeSftp := setupSftp(fakeSsh, nil)
	_ = fakeSftp
	p := NewClientPool(time.Minute, time.Minute)

	id1 := ClientID{
		Hostname: "testhost",
		Username: "auser",
		Password: "apassword",
	}
	id2 := ClientID{
		Hostname: "testhost2",
		Username: "auser",
		Password: "apassword",
	}

	c1, err := p.Get(id1)
	if err != nil {
		t.Fatal("Unexpected error from Dial", err)
	}

	c2, err := p.Get(id2)
	if err != nil {
		t.Fatal("Unexpected error from Dial", err)
	}

	if c1 == c2 {
		t.Fatal("c1 == c2")
	}

	if fakeSsh.dialCount != 2 {
		t.Fatalf("dialCount expected=2 actual=%d", fakeSsh.dialCount)
	}

	// cache miss
	c3, err := p.Get(id1)
	if err != nil {
		t.Fatal("Unexpected error from Dial", err)
	}
	if c3 == c1 {
		t.Fatal("c1 == c3")
	}

	if fakeSsh.dialCount != 3 {
		t.Fatalf("dialCount expected=3 actual=%d", fakeSsh.dialCount)
	}

	// release first client
	p.Put(id1, c1)

	// cache hit
	c4, err := p.Get(id1)
	if err != nil {
		t.Fatal("Unexpected error from Dial", err)
	}
	if c4 != c1 {
		t.Fatal("c1 != c4")
	}

	if fakeSsh.dialCount != 3 {
		t.Fatalf("dialCount expected=3 actual=%d", fakeSsh.dialCount)
	}
}

func poolGet(t *testing.T, p *ClientPool, id ClientID, s string) *Client {
	c, err := p.Get(id)
	if err != nil {
		t.Fatalf("[%s] get failed: %s", s, err)
	}
	return c
}

func TestPoolFlush(t *testing.T) {
	fSsh := setupSsh(nil)
	fSftp := setupSftp(fSsh, nil)
	_ = fSftp
	p := NewClientPool(time.Minute, time.Minute)

	id1 := ClientID{
		Hostname: "testhost",
		Username: "auser",
		Password: "apassword",
	}
	id2 := ClientID{
		Hostname: "testhost2",
		Username: "auser",
		Password: "apassword",
	}

	c1 := poolGet(t, p, id1, "c1")
	c2 := poolGet(t, p, id2, "c2")
	c3 := poolGet(t, p, id1, "c3")
	c4 := poolGet(t, p, id1, "c4")
	_ = c4
	fmt.Printf("c1=%p\nc2=%p\nc3=%p\nc4=%p\n", c1, c2, c3, c4)

	// release c1, c2 and c3 and mark c1 only as expired
	p.Put(id1, c1)
	p.Put(id2, c2)
	p.Put(id1, c3)
	c1.lastUsed = time.Now().Add(-time.Hour)
	p.flush()

	// a fetch for id1 should return c3
	c5 := poolGet(t, p, id1, "c5")
	if c5 != c3 {
		t.Fatal("c5 != c3")
	}

	p.Put(id1, c4)
	p.Put(id1, c5)

	// should get c4 back as the next oldest
	c6 := poolGet(t, p, id1, "c6")
	if c6 != c4 {
		t.Fatalf("c6 != c4 got %p", c6)
	}

	if !fSsh.closeCalled {
		t.Errorf("close not called on ssh client")
	}
}

type retryClient struct {
	SftpClient
	mkdirErrors []error
}

func (rc *retryClient) Mkdir(path string) (err error) {
	if len(rc.mkdirErrors) > 0 {
		err, rc.mkdirErrors = rc.mkdirErrors[0], rc.mkdirErrors[1:]
	}
	return err
}

// first attempt will return io.EOF; should trigger a request
// for a new client and then re-run the command
func TestPoolRetryOk(t *testing.T) {
	c := &fakeSftp{}
	setupSsh(nil)
	newCount := 0
	sftpNewClient = func(conn sshClient) (SftpClient, error) {
		newCount++
		return c, nil
	}

	p := NewClientPool(time.Minute, time.Minute)

	id := ClientID{
		Hostname: "testhost",
		Username: "auser",
		Password: "apassword",
	}

	i := 0
	c1 := poolGet(t, p, id, "c1")
	c2, err := p.Retry(c1, func(c *Client) error {
		if i < 1 {
			i++
			return io.EOF
		}
		return nil
	})
	if c2 == nil {
		t.Fatal("c2 == nil")
	}
	if err != nil {
		t.Fatal("Unexpected error", err)
	}

	if newCount != 2 {
		t.Errorf("incorrected newCount expected=2 actual=%d", newCount)
	}
}

// Return io.EOF for all clients; should  make maxRetries attempts and then hard fail
func TestPoolRetryFail(t *testing.T) {
	c := &fakeSftp{}
	setupSsh(nil)
	newCount := 0
	sftpNewClient = func(conn sshClient) (SftpClient, error) {
		newCount++
		return c, nil
	}

	p := NewClientPool(time.Minute, time.Minute)

	id := ClientID{
		Hostname: "testhost",
		Username: "auser",
		Password: "apassword",
	}

	i := 0
	c1 := poolGet(t, p, id, "c1")
	c2, err := p.Retry(c1, func(c *Client) error {
		if i < maxRetries+10 {
			i++
			return io.EOF
		}
		return nil
	})
	if c2 != nil {
		t.Fatal("c2 != nil")
	}
	if err != io.EOF {
		t.Fatal("Unexpected error", err)
	}

	if newCount != maxRetries+1 { // include initial get request
		t.Errorf("incorrected newCount expected=%d actual=%d", maxRetries, newCount)
	}
}

// Check that a non EOF error is passed through ok (ie. no retry needed)
func TestPoolRetryReturnError(t *testing.T) {
	c := &fakeSftp{}
	setupSsh(nil)
	newCount := 0
	sftpNewClient = func(conn sshClient) (SftpClient, error) {
		newCount++
		return c, nil
	}

	p := NewClientPool(time.Minute, time.Minute)

	id := ClientID{
		Hostname: "testhost",
		Username: "auser",
		Password: "apassword",
	}

	retErr := errors.New("Geeneric error")
	c1 := poolGet(t, p, id, "c1")
	c2, err := p.Retry(c1, func(c *Client) error {
		return retErr
	})
	if c2 == nil {
		t.Fatal("c2 == nil")
	}
	if err != retErr {
		t.Fatal("Unexpected error", err)
	}

	if newCount != 1 { // should of been no retries
		t.Errorf("incorrected newCount expected=1 actual=%d", newCount)
	}
}

func TestRetryableClient(t *testing.T) {
	// fetch a retryable client
	// force an EOF response
	// ensure retry/connect
	// return to pool

	var callCount int
	c := &fakeSftp{
		getWDFunc: func() (string, error) {
			callCount++
			if callCount < 2 {
				// simulate dropped connection
				return "", io.EOF
			}
			return "ok", nil
		},
	}
	setupSsh(nil)
	newCount := 0
	sftpNewClient = func(conn sshClient) (SftpClient, error) {
		newCount++
		return c, nil
	}

	p := NewClientPool(time.Minute, time.Minute)

	id := ClientID{
		Hostname: "testhost",
		Username: "auser",
		Password: "apassword",
	}

	client, err := p.GetRetryable(id)
	if err != nil {
		t.Fatal("Pool.GetRetryable failed", err)
	}

	wd, err := client.Getwd()
	if err != nil {
		t.Fatal("Unexpected error", err)
	}

	if wd != "ok" {
		t.Error("ok not returned from client")
	}

	if newCount != 2 {
		t.Error("Expected two clients to be created; got", newCount)
	}

	if callCount != 2 {
		t.Error("Expected two calls to getwd; got", newCount)
	}

	p.PutRetryable(id, client)
	if count := len(p.idleClients); count != 1 {
		t.Error("Incorrect idleClient count", count)
	}

	if p.clientCount != 1 {
		t.Error("Incorrect clientCount ", p.clientCount)
	}
	if p.activeCount != 0 {
		t.Error("Incorrect activeCount ", p.activeCount)
	}
}
