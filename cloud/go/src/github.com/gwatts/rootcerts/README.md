# Root CA Certificates for Go

[![GoDoc](https://godoc.org/github.com/gwatts/rootcerts?status.svg)](https://godoc.org/github.com/gwatts/rootcerts)
[![Build Status](https://travis-ci.org/gwatts/rootcerts.svg?branch=master)](https://travis-ci.org/gwatts/rootcerts)

This project converts root certificate authority certificates provided by the
Mozilla project into a .go package that can be statically compiled into a Go
project.

## Motivation

Many Go programs need to access remote SSL/TLS services (eg. over https).
The crypto/tls package validates remote certificates by loading public CA
certificates from the filesystem as provided by the operating system
(eg. in /etc/ssl).

This works well, unless the program is running in any environment where these
certificates are not installed, are not accessible, or are out of date.  This
commonly occurs when running Go programs in a minimal docker container or the 
binary is distributed as-is to end users who may or may not be running the program
on a machine with up to date certificates installed/accessible.

Compiling the root certificates into the program provides guaranteed stable
access to them without external dependencies.

## Alternatives

Instead of compiling the certificates into the binary, one could:

* Install packages at the OS level and mount /etc/ssl as a volume for the
container
* Add a certificates layer to the image when defining the Dockerfile

Compiling the certificates may, however, be simpler to deploy in many cases and
result in a predictable outcome whether the binary is executed in a container
or on a host.

## Usage

You may either use the certificates provided at the root level of this project,
which are periodically refreshed, or use the gencerts tool to create a
rootcerts.go file that may be copied into your project.

Calling the `UpdateDefaultTransport` method will make the certificates available
to the default http transport, which is sufficient for many projects.

### Using gencerts

The gencerts tool reads a certdata.txt file, either from the local filesystem,
or directly from the Mozilla Mercurial site (though note, it uses https by
default so does itself require local ca certificates!)

Note also that the format of certdata.txt changes occasionally, which may break
the gencerts tool.  Relying on -download for a production build process may
thus be a bad idea!

```bash
gencerts -download -package mypackage -target rootcerts.go
```

gencerts will generate a rootcerts.go and also a rootcerts_16.go if there are 
any certificate with a negative serial number.  Only Go version 1.6 and later
supports such certificates, so rootcerts_16.go uses a build flag to ensure
compatibility with older versions of Go.

## Other Notes

gencerts only outputs certificates that the certdata.txt file has labeled as
a trusted delegator (ie. certificates that are suitable for use a certificate
authority).

Certificates may be marked as trusted for servers, email or code signing.

## Useful Resources

Some of the information I came across while writing this tool:

* [https://github.com/ralphholz/root-store-archaeology]
* [http://curl.haxx.se/cvssource/lib/mk-ca-bundle.pl]
* Ubuntu ca-certificates package (certdata2pem.py)


