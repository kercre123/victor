package chipperfunction

import (
	"fmt"
	"github.com/stretchr/testify/assert"
	"testing"
)

var testNames = []struct {
	textName, dfName string
}{
	{"LeBron James", "Lebron James"},
	{"doNald", "Donald"},
	{"MaRy", "Mary"},
	{"A Tribe Called quest", "a tribe called quest"},
}

func TestCorrectNameCase(t *testing.T) {
	for _, tst := range testNames {
		q := fmt.Sprintf("My name is %s", tst.textName)
		newName := correctUsernameCase(q, tst.dfName)
		assert.Equal(t, tst.textName, newName)
	}
}
