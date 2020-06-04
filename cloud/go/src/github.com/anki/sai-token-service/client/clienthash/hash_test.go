package clienthash

import (
	"fmt"
	"github.com/satori/go.uuid"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestHelper_newSalt(t *testing.T) {
	salt, err := newSalt()
	require.NoError(t, err)
	assert.Len(t, salt, saltSize)
	fmt.Println(salt)
}

func TestHelper_hash(t *testing.T) {
	id, _ := uuid.NewV4()
	salt, err := newSalt()
	require.NoError(t, err)
	assert.Len(t, id.Bytes(), tokenSize)

	h := hash(id.Bytes(), salt)
	assert.Len(t, h, hashSize)
}

func TestHelper_newFromToken(t *testing.T) {
	id, _ := uuid.NewV4()

	h, err := newFromToken(id.Bytes())
	require.NoError(t, err)
	require.NotNil(t, h)
	assert.Len(t, h.hash, hashSize)
	assert.Len(t, h.salt, saltSize)

	newHash := hash(id.Bytes(), h.salt)
	assert.Equal(t, h.hash, newHash)
}

func Test_BasicRoundTrip(t *testing.T) {
	token, hash, err := GenerateClientTokenAndHash()
	fmt.Println(token)
	fmt.Println(hash)
	require.NoError(t, err)
	assert.NotEmpty(t, token)
	assert.NotEmpty(t, hash)
	require.NoError(t, CompareHashAndToken(hash, token))
}
