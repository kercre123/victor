package stsgen

import "encoding/json"

type policyDocument struct {
	Version   string
	Statement []statementEntry
}

func (p policyDocument) toJson() (string, error) {
	buf, err := json.Marshal(&p)
	return string(buf), err
}

type statementEntry struct {
	Effect   string
	Action   []string
	Resource []string
}

