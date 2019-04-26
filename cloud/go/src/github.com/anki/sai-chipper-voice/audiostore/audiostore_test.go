package audiostore

import (
	"github.com/stretchr/testify/assert"
	"testing"
)

func TestMetadata(t *testing.T) {
	m := metadata{md: make(map[string]string)}
	m.addIfNotEmpty("field1", "field1")
	m.addIfNotEmpty("emptyfield", "")

	val, ok := m.md["field1"]
	assert.Equal(t, true, ok)
	assert.Equal(t, "field1", val)

	_, ok = m.md["emptyfield"]
	assert.Equal(t, false, ok)
}
