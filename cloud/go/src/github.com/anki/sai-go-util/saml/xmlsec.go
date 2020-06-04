package saml


const (
	ErrXmlSecFailed             = 1
	ErrMallocFailed             = 2
	ErrStdupFailed              = 3
	ErrCryptoFailed             = 4
	ErrXmlFailed                = 5
	ErrXsltFailed               = 6
	ErrIOFailed                 = 7
	ErrDisabled                 = 8
	ErrNotImplemented           = 9
	ErrInvalidSize              = 11
	ErrInvalidData              = 12
	ErrInvalidResult            = 13
	ErrInvalidType              = 14
	ErrInvalidOperation         = 15
	ErrInvalidStatus            = 16
	ErrInvalidFormat            = 17
	ErrDataNotMatch             = 18
	ErrInvalidNode              = 21
	ErrInvalidNodeContent       = 22
	ErrInvalidNodeAttribute     = 23
	ErrMissingNodeAttribute     = 25
	ErrNodeAlreadyPresent       = 26
	ErrUnexpectedNode           = 27
	ErrNodeNotFound             = 28
	ErrInvalidTransform         = 31
	ErrInvalidTransformKey      = 32
	ErrInvalidURIType           = 33
	ErrTransformSameDocRequired = 34
	ErrTransformDisabled        = 35
	ErrInvalidKeyData           = 41
	ErrKeyDataNotFound          = 42
	ErrKeyDataAlreadyExist      = 43
	ErrInvalidKeyDataSize       = 44
	ErrKeyNotFound              = 45
	ErrKeyDataDisabled          = 46
	ErrMaxRetrievalsLevel       = 51
	ErrMaxRetrievalTypeMismatch = 52
	ErrMaxEncKeylevel           = 61
	ErrCertVerifyFailed         = 71
	ErrCertNotFound             = 72
	ErrCertRevoked              = 73
	ErrCertIssuerFailed         = 74
	ErrCertNotYetValid          = 75
	ErrCertHasExpired           = 76
	ErrDsigNoReferences         = 81
	ErrDsigInvalidReference     = 82
	ErrAssertion                = 100
	ErrMaxNumber                = 256

	ErrUnableToParse       = 1000
	ErrNoStartNode         = 1001
	ErrIdDefFailed         = 1002
	ErrXmlSecContextFailed = 1003
	ErrPubKeyReadFailed    = 1004
	ErrNotXmlDigFormat     = 1005
	ErrInitFailed          = 1006
	ErrBadVersion          = 1007
	ErrCryptoLoadFailed    = 1008
	ErrCryptoInitFailed    = 1009

	ErrNotInitialized = 10000
	ErrNotAvailable   = 10001
)

var (
	NotInitialized = &XmlSecError{
		Code: ErrNotInitialized,
		Msg:  "Library is not initialized",
	}
	NotAvailable = &XmlSecError{
		Code: ErrNotInitialized,
		Msg:  "XMLSec verification not available",
	}
)

type XmlSecError struct {
	Code int
	Msg  string
}

func (e *XmlSecError) Error() string {
	return e.Msg
}
