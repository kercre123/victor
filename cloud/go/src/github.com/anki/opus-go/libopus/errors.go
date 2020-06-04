// Copyright © 2015-2017 Go Opus Authors (see AUTHORS file)
//
// License for use of this code is detailed in the LICENSE file

package opus

import (
	"fmt"
)

/*
#cgo LDFLAGS: -lopus
#include <opus.h>

// Access the preprocessor from CGO

// Errors for libopus
const int CONST_OPUS_OK = OPUS_OK;
const int CONST_OPUS_BAD_ARG = OPUS_BAD_ARG;
const int CONST_OPUS_BUFFER_TOO_SMALL = OPUS_BUFFER_TOO_SMALL;
const int CONST_OPUS_INTERNAL_ERROR = OPUS_INTERNAL_ERROR;
const int CONST_OPUS_INVALID_PACKET = OPUS_INVALID_PACKET;
const int CONST_OPUS_UNIMPLEMENTED = OPUS_UNIMPLEMENTED;
const int CONST_OPUS_INVALID_STATE = OPUS_INVALID_STATE;
const int CONST_OPUS_ALLOC_FAIL = OPUS_ALLOC_FAIL;
*/
import "C"

type Error int

var _ error = Error(0)

// Libopus errors
var (
	ErrOK             = Error(C.CONST_OPUS_OK)
	ErrBadArg         = Error(C.CONST_OPUS_BAD_ARG)
	ErrBufferTooSmall = Error(C.CONST_OPUS_BUFFER_TOO_SMALL)
	ErrInternalError  = Error(C.CONST_OPUS_INTERNAL_ERROR)
	ErrInvalidPacket  = Error(C.CONST_OPUS_INVALID_PACKET)
	ErrUnimplemented  = Error(C.CONST_OPUS_UNIMPLEMENTED)
	ErrInvalidState   = Error(C.CONST_OPUS_INVALID_STATE)
	ErrAllocFail      = Error(C.CONST_OPUS_ALLOC_FAIL)
)

// Error string (in human readable format) for libopus errors.
func (e Error) Error() string {
	return fmt.Sprintf("opus: %s", C.GoString(C.opus_strerror(C.int(e))))
}
