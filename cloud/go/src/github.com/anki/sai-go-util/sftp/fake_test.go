// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package sftp

import (
	"bytes"
	"errors"
	"os"
	"strings"
	"time"

	"github.com/anki/sai-go-util"
	"golang.org/x/crypto/ssh"
)

var baseTime = util.Must2(time.Parse(time.RFC3339, "2014-04-18T15:00:00Z")).(time.Time)

// fake ssh/sftp clients

type fakeKey struct {
	marshalOut []byte
}

func (k *fakeKey) Marshal() []byte {
	return k.marshalOut
}

func (k *fakeKey) Type() string { return "fakekey" }

func (k *fakeKey) Verify(data []byte, sig *ssh.Signature) error { return nil }

var (
	fakeKey1            = &fakeKey{[]byte("test")}
	fakeKey1Fingerprint = "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3"
)

type fakeSsh struct {
	dialCount       int
	recvNetwork     string
	recvAddr        string
	recvConfig      *ssh.ClientConfig
	sshDialError    error
	sftpClientError error
	closeCalled     bool
}

func (f *fakeSsh) Close() error {
	f.closeCalled = true
	return nil
}

func setupSsh(errResponse error) *fakeSsh {
	fs := &fakeSsh{
		sshDialError: errResponse,
	}

	key := fakeKey1

	sshDial = func(network, addr string, config *ssh.ClientConfig) (sshClient, error) {
		fs.dialCount++
		fs.recvNetwork = network
		fs.recvAddr = addr
		fs.recvConfig = config
		if fs.sshDialError != nil {
			return nil, fs.sshDialError
		}
		config.HostKeyCallback("", nil, key)
		return fs, nil
	}
	return fs
}

type fakeSftp struct {
	newClientError  error
	closeCalled     bool
	createPath      string
	createError     error
	createBuf       bytes.Buffer
	mkdirPath       string
	mkdirError      error
	openPath        string
	openError       error
	openData        []byte // data to return for file reads
	readDirError    error
	readDirResponse []os.FileInfo
	removeError     error
	removePath      string
	renameOldName   string
	renameNewName   string
	renameError     error
	getWDFunc       func() (string, error)
}

func setupSftp(fakeSsh sshClient, errResponse error) *fakeSftp {
	fs := &fakeSftp{
		newClientError: errResponse,
	}
	sftpNewClient = func(conn sshClient) (SftpClient, error) {
		if fs.newClientError != nil {
			return nil, fs.newClientError
		}
		return fs, nil
	}
	return fs
}

// make bytes.Buffer comply with the sftpFile interface
type createFile struct {
	*bytes.Buffer
}

func (f *createFile) Close() error                                 { return nil }
func (f *createFile) Seek(offset int64, whence int) (int64, error) { return 0, nil }
func (f *createFile) Stat() (os.FileInfo, error)                   { return nil, errors.New("Unsupported") }

type readFile struct {
	*bytes.Reader
	isDir   bool
	name    string
	size    int64
	modTime time.Time
}

func (f *readFile) Stat() (os.FileInfo, error) {
	return f, nil
}

// implement FileInfo interface

func (f *readFile) Close() error { return nil }
func (f *readFile) Name() string { return f.name }
func (f *readFile) Mode() os.FileMode {
	if f.isDir {
		return os.ModeDir | 0777
	}
	return 0777
}

func (f *readFile) Size() int64 {
	if f.Reader != nil {
		return int64(f.Reader.Len())
	}
	return f.size
}

func (f *readFile) ModTime() time.Time { return f.modTime }
func (f *readFile) IsDir() bool        { return f.isDir }
func (f *readFile) Sys() interface{}   { return nil }
func (f *readFile) Write(b []byte) (int, error) {
	return 0, errors.New("readFile does not support Write")
}

func fEntry(name string, size int64, mtime time.Time) *readFile {
	return &readFile{isDir: false, name: name, size: size, modTime: mtime}
}

func dEntry(name string, mtime time.Time) *readFile {
	return &readFile{isDir: true, name: name, modTime: mtime}
}

func (fs *fakeSftp) Close() error {
	fs.closeCalled = true
	return nil
}

func (fs *fakeSftp) Create(path string) (SftpFile, error) {
	fs.createPath = path
	if fs.createError != nil {
		return nil, fs.createError
	}
	return &createFile{&fs.createBuf}, nil
}

func (fs *fakeSftp) Stat(path string) (os.FileInfo, error) {
	return nil, errors.New("Unsupported")
}

func (fs *fakeSftp) Getwd() (string, error) {
	if fs.getWDFunc != nil {
		return fs.getWDFunc()
	}
	return "/", nil
}

func (fs *fakeSftp) Open(path string) (SftpFile, error) {
	fs.openPath = path
	if fs.openError != nil {
		return nil, fs.openError
	}
	return &readFile{
		Reader:  bytes.NewReader(fs.openData),
		isDir:   strings.Contains(path, "-dir"),
		name:    path,
		modTime: util.Must2(time.Parse(time.RFC3339, "2014-04-18T15:00:00Z")).(time.Time),
	}, nil
}

func (fs *fakeSftp) Mkdir(path string) error {
	fs.mkdirPath = path
	return fs.mkdirError
}

func (fs *fakeSftp) ReadDir(p string) ([]os.FileInfo, error) {
	if fs.readDirError != nil {
		return nil, fs.readDirError
	}
	return fs.readDirResponse, nil
}

func (fs *fakeSftp) Remove(path string) error {
	fs.removePath = path
	return fs.removeError
}

func (fs *fakeSftp) Rename(oldname, newname string) error {
	fs.renameOldName, fs.renameNewName = oldname, newname
	return fs.renameError
}

func (fs *fakeSftp) Join(elem ...string) string {
	return strings.Join(elem, "/")
}
