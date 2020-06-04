// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

// Package sftp provides a wrapper for the github.com/pkg/sftp client that
// adds connection pooling and retry support, along with metrics collection.
package sftp

import (
	"crypto/sha1"
	"fmt"
	"io"
	"net"
	"os"
	"sync"
	"time"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/metricsutil"
	"github.com/anki/sai-go-util/strutil"
	"github.com/pkg/sftp"
	"golang.org/x/crypto/ssh"
)

// the sftp package doesn't expose these constants
const (
	NoSuchFile       = 2
	PermissionDenied = 3
	OpUnsupported    = 8
)

// Default timeout for new SSH connections used during SFTP client creation
var ConnectTimeout = 30 * time.Second

// used by Retry()
var (
	maxRetries = 3
	retryDelay = 250 * time.Millisecond
)

// ClientID is used to uniquely identify a user login on a specific
// remote SFTP server.
type ClientID struct {
	// Hostname is the remote address of the SFTP server to connect
	// to. If a non-default port is desired, the string should be in
	// "hostname:<port>" format.
	Hostname string

	// Fingerprint specifies the hash of the public key of the remote
	// server, for validation. If the string is left empty, no
	// validation will be performed.
	Fingerprint string

	// Username is the name of the remote user to use for logging in
	// to the SFTP server.
	Username string

	// Password is the un-hashed (plain text) password of the remote
	// user.
	Password string
}

// produce a hash/fingerprint of a key.
// This will not match the opeenssh format; more work than i want to do.
func hashKey(key ssh.PublicKey) string {
	return fmt.Sprintf("%x", sha1.Sum(key.Marshal()))
}

// sshDial provides an entry point for the actual dialing of SSH
// connections to be replaced by unit tests.
var sshDial = func(network, addr string, config *ssh.ClientConfig) (sshClient, error) {
	conn, err := net.DialTimeout(network, addr, config.Timeout)
	if err != nil {
		return nil, err
	}
	if config.Timeout > 0 {
		conn.SetDeadline(time.Now().Add(config.Timeout))
	}
	c, chans, reqs, err := ssh.NewClientConn(conn, addr, config)
	if err != nil {
		return nil, err
	}
	if config.Timeout > 0 {
		conn.SetDeadline(time.Time{})
	}
	return ssh.NewClient(c, chans, reqs), nil
}

// sftpNewClient provides an internal constructor to be replaced by
// unit tests.
var sftpNewClient = func(conn sshClient) (SftpClient, error) {
	c, err := sftp.NewClient(conn.(*ssh.Client))
	return &wrapSftp{c}, err
}

// SftpFile defines an explicit interface for the methods defined by
// the sftp.File type exposed by the github.com/pkg/sftp package.
type SftpFile interface {
	Close() error
	Read(b []byte) (int, error)
	Seek(offset int64, whence int) (int64, error)
	Stat() (os.FileInfo, error)
	Write(b []byte) (int, error)
}

// SftpClient defines an explicit interface for the methods defined by
// the sftp.Client type exposed by the github.com/pkg/sftp package.
type SftpClient interface {
	Close() error
	Create(path string) (SftpFile, error)
	Mkdir(path string) error
	Stat(p string) (os.FileInfo, error)
	Getwd() (string, error)
	Open(path string) (SftpFile, error)
	ReadDir(p string) ([]os.FileInfo, error)
	Remove(path string) error
	Rename(oldname, newname string) error
	Join(elem ...string) string
}

// ----------------------------------------------------------------------
// SFTP client wrapped with metrics
// ----------------------------------------------------------------------

var (
	registry       = metricsutil.NewRegistry("sftp")
	clientRegistry = metricsutil.NewChildRegistry(registry, "client")

	timerAll     = metricsutil.GetTimer("All", clientRegistry)
	timerClose   = metricsutil.GetTimer("CloseLatency", clientRegistry)
	errorClose   = metricsutil.GetCounter("CloseError", clientRegistry)
	timerStat    = metricsutil.GetTimer("StatLatency", clientRegistry)
	errorStat    = metricsutil.GetCounter("StatError", clientRegistry)
	timerGetwd   = metricsutil.GetTimer("GetwdLatency", clientRegistry)
	errorGetwd   = metricsutil.GetCounter("GetwdError", clientRegistry)
	timerCreate  = metricsutil.GetTimer("CreateLatency", clientRegistry)
	errorCreate  = metricsutil.GetCounter("CreateError", clientRegistry)
	timerMkdir   = metricsutil.GetTimer("MkdirLatency", clientRegistry)
	errorMkdir   = metricsutil.GetCounter("MkdirError", clientRegistry)
	timerOpen    = metricsutil.GetTimer("OpenLatency", clientRegistry)
	errorOpen    = metricsutil.GetCounter("OpenError", clientRegistry)
	timerReadDir = metricsutil.GetTimer("ReadDirLatency", clientRegistry)
	errorReadDir = metricsutil.GetCounter("ReadDirError", clientRegistry)
	timerRemove  = metricsutil.GetTimer("RemoveLatency", clientRegistry)
	errorRemove  = metricsutil.GetCounter("RemoveError", clientRegistry)
	timerRename  = metricsutil.GetTimer("RenameLatency", clientRegistry)
	errorRename  = metricsutil.GetCounter("RenameError", clientRegistry)
)

type wrapSftp struct {
	*sftp.Client
}

func (w *wrapSftp) Create(path string) (SftpFile, error) {
	start := time.Now()
	defer timerAll.UpdateSince(start)
	defer timerCreate.UpdateSince(start)

	f, err := w.Client.Create(path)
	if err != nil {
		errorCreate.Inc(1)
		return nil, err
	}
	return f, nil
}

func (w *wrapSftp) Open(path string) (SftpFile, error) {
	start := time.Now()
	defer timerAll.UpdateSince(start)
	defer timerOpen.UpdateSince(start)

	f, err := w.Client.Open(path)
	if err != nil {
		errorOpen.Inc(1)
		return nil, err
	}
	return f, nil
}

func (w *wrapSftp) Close() error {
	start := time.Now()
	defer timerAll.UpdateSince(start)
	defer timerClose.UpdateSince(start)

	err := w.Client.Close()
	if err != nil {
		errorClose.Inc(1)
	}
	return err
}

func (w *wrapSftp) Stat(p string) (os.FileInfo, error) {
	start := time.Now()
	defer timerAll.UpdateSince(start)
	defer timerStat.UpdateSince(start)

	info, err := w.Client.Stat(p)
	if err != nil {
		errorStat.Inc(1)
	}
	alog.Debug{"command": "sftp_stat", "path": p, "success": err == nil}.Log()
	return info, err
}

func (w *wrapSftp) Getwd() (string, error) {
	start := time.Now()
	defer timerAll.UpdateSince(start)
	defer timerGetwd.UpdateSince(start)

	dir, err := w.Client.Getwd()
	if err != nil {
		errorGetwd.Inc(1)
	}
	alog.Debug{"command": "sftp_getwd", "success": err == nil}.Log()
	return dir, err
}

func (w *wrapSftp) Mkdir(path string) error {
	start := time.Now()
	defer timerAll.UpdateSince(start)
	defer timerMkdir.UpdateSince(start)

	err := w.Client.Mkdir(path)
	if err != nil {
		errorMkdir.Inc(1)
	}
	alog.Debug{"command": "sftp_mkdir", "path": path, "success": err == nil}.Log()
	return err
}

func (w *wrapSftp) ReadDir(p string) ([]os.FileInfo, error) {
	start := time.Now()
	defer timerAll.UpdateSince(start)
	defer timerReadDir.UpdateSince(start)

	info, err := w.Client.ReadDir(p)
	if err != nil {
		errorReadDir.Inc(1)
	}
	alog.Debug{"command": "sftp_readdir", "path": p, "success": err == nil, "length": len(info)}.Log()
	return info, err
}

func (w *wrapSftp) Remove(path string) error {
	start := time.Now()
	defer timerAll.UpdateSince(start)
	defer timerRemove.UpdateSince(start)

	err := w.Client.Remove(path)
	if err != nil {
		errorRemove.Inc(1)
	}
	alog.Debug{"command": "sftp_remove", "path": path, "success": err == nil}.Log()
	return err
}

func (w *wrapSftp) Rename(oldname, newname string) error {
	start := time.Now()
	defer timerAll.UpdateSince(start)
	defer timerRename.UpdateSince(start)

	err := w.Client.Rename(oldname, newname)
	if err != nil {
		errorRename.Inc(1)
	}
	alog.Debug{"command": "sftp_rename", "from": oldname, "to": newname, "success": err == nil}.Log()
	return err
}

type sshClient interface {
	Close() error
}

var (
	poolRegistry             = metricsutil.NewChildRegistry(registry, "pool")
	errorDial                = metricsutil.GetCounter("DialError", poolRegistry)
	errorFingerprintMismatch = metricsutil.GetCounter("FingerprintMismatchError", poolRegistry)
	newClientCount           = metricsutil.GetCounter("NewClient", clientRegistry)
	poolReuseCount           = metricsutil.GetCounter("ReuseClient", poolRegistry)
	poolCreateCount          = metricsutil.GetCounter("CreateClient", poolRegistry)
	poolReleaseCount         = metricsutil.GetCounter("ReleaseClient", poolRegistry)
)

// Client is a concrete implementation of the SftpClient interface
// wrapping the client provided by the sftp package with metrics and
// enabling client pooling.
type Client struct {
	SftpClient
	sshClient   sshClient
	id          ClientID
	cid         string
	lastUsed    time.Time
	fingerprint string
	dead        bool
}

// NewClient returns a new SFTP client connected to the specified SFTP endpoint.
func NewClient(id ClientID) (*Client, error) {
	// connect a new client
	var fingerprint string
	config := &ssh.ClientConfig{
		User: id.Username,
		Auth: []ssh.AuthMethod{
			ssh.Password(id.Password),
		},
		HostKeyCallback: func(hostname string, remote net.Addr, key ssh.PublicKey) error {
			fingerprint = hashKey(key)
			if id.Fingerprint != "" && id.Fingerprint != fingerprint {
				errorFingerprintMismatch.Inc(1)
				return fmt.Errorf("Host fingerprint does not match requested: %q", fingerprint)
			}
			return nil
		},
		Timeout: ConnectTimeout,
	}

	sc, err := sshDial("tcp", id.Hostname, config)
	if err != nil {
		errorDial.Inc(1)
		return nil, err
	}

	sfc, err := sftpNewClient(sc)
	if err != nil {
		return nil, err
	}
	cid, _ := strutil.ShortLowerUUID()
	newClientCount.Inc(1)
	return &Client{
		SftpClient:  sfc,
		sshClient:   sc,
		id:          id,
		cid:         cid,
		lastUsed:    time.Now(),
		fingerprint: fingerprint,
	}, nil
}

// Close closes the underlying ssh connection.
func (c *Client) Close() error {
	return c.sshClient.Close()
}

// ClientPool maintains a pool of recently idled SFTP Clients, ready
// for re-use.
type ClientPool struct {
	maxIdleTime   time.Duration
	flushInterval time.Duration

	m           sync.Mutex
	idleClients map[ClientID][]*Client
	clientCount int
	activeCount int
}

// NewClientPool initializes a client pool and starts a background flusher
func NewClientPool(maxIdleTime, flushInterval time.Duration) *ClientPool {
	cp := &ClientPool{
		maxIdleTime:   maxIdleTime,
		flushInterval: flushInterval,
		idleClients:   make(map[ClientID][]*Client),
	}
	go cp.flusher()
	return cp
}

// Get a connected client from the idle pool, or connect one if none available
func (cp *ClientPool) Get(id ClientID) (c *Client, err error) {
	cp.m.Lock()

	if idle := cp.idleClients[id]; len(idle) > 0 {
		c, cp.idleClients[id] = idle[0], idle[1:]
		cp.activeCount++
		cp.m.Unlock()
		poolReuseCount.Inc(1)
		return c, nil
	}
	cp.m.Unlock()

	c, err = NewClient(id)
	if err == nil {
		cp.m.Lock()
		cp.activeCount++
		cp.clientCount++
		poolCreateCount.Inc(1)
		cp.m.Unlock()
	}
	return c, err
}

// Put returns a client back to the pool
func (cp *ClientPool) Put(id ClientID, client *Client) {
	if client.dead {
		// Retry has previously declared the client as dead; discard.
		return
	}

	cp.m.Lock()
	defer cp.m.Unlock()

	poolReleaseCount.Inc(1)
	client.lastUsed = time.Now()
	cp.idleClients[id] = append(cp.idleClients[id], client)
	cp.activeCount--
}

// GetRetryable fetches a RetryableClient from the pool.  This a wrapper for a regular
// Client that will auto-reconnect to the server if an operation is requested
// after the connection has been dropped.
func (cp *ClientPool) GetRetryable(id ClientID) (rc *RetryableClient, err error) {
	c, err := cp.Get(id)
	if err != nil {
		return nil, err
	}
	return &RetryableClient{pool: cp, client: c}, nil
}

// PutRetryable returns a RetryableClient to the pool.
func (cp *ClientPool) PutRetryable(id ClientID, client *RetryableClient) {
	cp.Put(id, client.client)
}

var (
	errorRetriesExhausted = metricsutil.GetCounter("RetryExhaustedError", registry)
)

// Retry runs a function and checks its return error code for io.EOF, which is returned
// from SFTP operations when the remote server has disconnected.
// If it receives io.EOF it discards the client and attempts to reconnet and re-execute
// the command up to maxRetries attempts.  Other errors from the command are passed through.
func (cp *ClientPool) Retry(client *Client, cmd func(c *Client) error) (c *Client, err error) {
	var i int
	for {
		i++
		if err = cmd(client); err != io.EOF {
			return client, err
		}
		if i > maxRetries {
			errorRetriesExhausted.Inc(1)
			return nil, err
		}

		// this client is dead
		client.dead = true
		cp.m.Lock()
		cp.activeCount--
		cp.clientCount--
		cp.m.Unlock()
		time.Sleep(retryDelay)

		// try to fetch a new client
		client, err = cp.Get(client.id)
		if err != nil {
			return nil, err
		}
	}
}

// flusher is a background goroutine that periodically calls flush()
func (cp *ClientPool) flusher() {
	for {
		time.Sleep(cp.flushInterval)
		cp.flush()
	}
}

// flush scans the list of idleClients and closes/removes those that have been
// idle too long.
func (cp *ClientPool) flush() {
	var prune, p []*Client

	// collect a list of expired clients to prune while the lock is held
	cp.m.Lock()
	for id, clients := range cp.idleClients {
		for i := len(clients); i > 0; i-- {
			client := clients[i-1]
			if time.Since(client.lastUsed) > cp.maxIdleTime {
				// as the idlelist is kept in order of least to most recently used
				// we can prune all of those older that the first we come acrosss that's expired
				p, cp.idleClients[id] = clients[:i], clients[i:]
				prune = append(prune, p...)
			}
		}
	}
	cp.clientCount -= len(prune)
	alog.Info{"action": "prunepool", "status": "ok", "pruned": len(prune), "total_active": cp.activeCount, "total_clients": cp.clientCount}.Log()
	cp.m.Unlock()

	// Don't need the lock to cleanly close connections
	for _, client := range prune {
		client.Close()
	}
}

// RetryableClient wraps a ClientPool and a Client.  Commands issued to
// a RetryableClient will check for io.EOF from the remote server, and
// automatically create a new client connection should the connection
// be lost before retrying the request.
type RetryableClient struct {
	pool   *ClientPool
	client *Client
}

var _ SftpClient = new(RetryableClient)

// Close wrapper.
func (rc *RetryableClient) Close() error {
	return rc.client.Close()
}

// Create wrapper.
func (rc *RetryableClient) Create(path string) (f SftpFile, err error) {
	rc.client, err = rc.pool.Retry(rc.client, func(c *Client) error {
		f, err = c.Create(path)
		return err
	})
	return f, err
}

// Mkdir wrapper.
func (rc *RetryableClient) Mkdir(path string) (err error) {
	rc.client, err = rc.pool.Retry(rc.client, func(c *Client) error {
		return c.Mkdir(path)
	})
	return err
}

// Stat wrapper.
func (rc *RetryableClient) Stat(p string) (fi os.FileInfo, err error) {
	rc.client, err = rc.pool.Retry(rc.client, func(c *Client) error {
		fi, err = c.Stat(p)
		return err
	})
	return fi, err
}

// Getwd wrapper.
func (rc *RetryableClient) Getwd() (wd string, err error) {
	rc.client, err = rc.pool.Retry(rc.client, func(c *Client) error {
		wd, err = c.Getwd()
		return err
	})
	return wd, err
}

// Open wrapper.
func (rc *RetryableClient) Open(path string) (f SftpFile, err error) {
	rc.client, err = rc.pool.Retry(rc.client, func(c *Client) error {
		f, err = c.Open(path)
		return err
	})
	return f, err
}

// ReadDir wrapper.
func (rc *RetryableClient) ReadDir(p string) (fi []os.FileInfo, err error) {
	rc.client, err = rc.pool.Retry(rc.client, func(c *Client) error {
		fi, err = c.ReadDir(p)
		return err
	})
	return fi, err
}

// Remove wrapper.
func (rc *RetryableClient) Remove(path string) (err error) {
	rc.client, err = rc.pool.Retry(rc.client, func(c *Client) error {
		return c.Remove(path)
	})
	return err
}

// Rename wrapper.
func (rc *RetryableClient) Rename(oldname, newname string) (err error) {
	rc.client, err = rc.pool.Retry(rc.client, func(c *Client) error {
		return c.Rename(oldname, newname)
	})
	return err
}

// Join wrapper.
func (rc *RetryableClient) Join(elem ...string) string {
	return rc.client.Join(elem...)
}
