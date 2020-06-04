package clienthash

import (
	"crypto/rand"
	"crypto/sha256"
	"crypto/subtle"
	"encoding/base64"
	"fmt"
	"io"

	"github.com/satori/go.uuid"
)

const (
	tokenSize = 16
	saltSize  = 16
	hashSize  = sha256.Size
)

var (
	errMismatchedTokenAndHash = fmt.Errorf("Hash mismatch")
	errHashTooLong            = fmt.Errorf("Hash too long")
	errHashTooShort           = fmt.Errorf("Hash too short")
	errTokenTooLong           = fmt.Errorf("Token too long")
	errTokenTooShort          = fmt.Errorf("Token too short")
)

// GenerateClientTokenAndHash generates a new random client token and
// returns it along with its hash.
func GenerateClientTokenAndHash() (string, string, error) {
	token, err := generateClientToken()
	if err != nil {
		return "", "", err
	}

	hash, err := generateSaltedHashFromClientToken(token)
	if err != nil {
		return "", "", err
	}

	return base64.StdEncoding.EncodeToString(token), base64.StdEncoding.EncodeToString(hash), nil
}

// generateSaltedHashFromClientToken returns a randomly salted hash of
// a plain text client token, with the salt appended to the raw
// hash. Modeled after the crypto/bcrypt interface.
func generateSaltedHashFromClientToken(token []byte) ([]byte, error) {
	if len(token) > tokenSize {
		return nil, errTokenTooLong
	} else if len(token) < tokenSize {
		return nil, errTokenTooShort
	}

	hashed, err := newFromToken(token)
	if err != nil {
		return nil, err
	}

	pair := make([]byte, 0, (hashSize + saltSize))
	pair = append(pair, hashed.hash...)
	pair = append(pair, hashed.salt...)

	return pair, nil
}

// CompareHashAndPassword compares a hashed client token with its
// possible plain text equivalent. Returns nil on success, or an error
// on failure. Modeled after the crypto/bcrypt interface.
func CompareHashAndToken(hashedToken, token string) error {
	// Decode the hash and the token to their raw bytes
	hashedBytes, err := base64.StdEncoding.DecodeString(hashedToken)
	if err != nil {
		return err
	}
	tokenBytes, err := base64.StdEncoding.DecodeString(token)
	if err != nil {
		return err
	}

	// extract the hash and salt from the pre-hashed value
	hashed, err := newFromHash(hashedBytes)
	if err != nil {
		return err
	}
	// hash the token using the salt extracted from the hashed input
	// and compare
	newHash := hash(tokenBytes, hashed.salt)

	if subtle.ConstantTimeCompare(hashed.hash, newHash) == 1 {
		return nil
	}

	return errMismatchedTokenAndHash
}

func generateClientToken() ([]byte, error) {
	token, err := uuid.NewV4()
	if err != nil {
		return nil, err
	}
	return token.Bytes(), nil
}

type hashed struct {
	// hash the hash of the token on its own (without the appended
	// salt)
	hash []byte
	// salt is the salt of the token
	salt []byte
}

func newFromToken(token []byte) (*hashed, error) {
	p := new(hashed)

	salt, err := newSalt()
	if err != nil {
		return nil, err
	}
	p.salt = salt
	p.hash = hash(token, salt)

	return p, nil
}

func newFromHash(hashedToken []byte) (*hashed, error) {
	if len(hashedToken) > (hashSize + saltSize) {
		return nil, errHashTooLong
	} else if len(hashedToken) < (hashSize + saltSize) {
		return nil, errHashTooShort
	}

	hash, salt := hashedToken[:hashSize], hashedToken[hashSize:]
	return &hashed{
		hash: hash,
		salt: salt,
	}, nil
}

func newSalt() ([]byte, error) {
	salt := make([]byte, saltSize)
	_, err := io.ReadFull(rand.Reader, salt)
	if err != nil {
		return nil, err
	}
	return salt, nil
}

func hash(token, salt []byte) []byte {
	salted := make([]byte, 0, (tokenSize + saltSize))
	salted = append(salted, token...)
	salted = append(salted, salt...)
	hash := sha256.Sum256(salted)
	return hash[:]
}
