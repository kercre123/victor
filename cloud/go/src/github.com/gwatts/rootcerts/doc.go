// Copyright 2015 Gareth Watts
// Licensed under an MIT license
// See the LICENSE file for details

/*
Package rootcerts provides a Go conversion of Mozilla's certdata.txt
file, extracting trusted CA certificates only.

It was generated using the gencerts tool using the following command line:
    gencerts -download -target rootcerts.go -package rootcerts

This package allows for the embedding of root CA certificates directly into
a Go executable, reducing or negating the need for Go to have access to root
certificates provided by the operating system in order to validate certificates
issued by those authorities.

Root certificates can be accessed through this package, or may be easily installed
into the http package's DefaultTransport by calling UpdateDefaultTransport.
*/
package rootcerts
