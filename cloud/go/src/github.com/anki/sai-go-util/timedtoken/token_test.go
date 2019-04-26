package timedtoken

import (
	"testing"
	"time"
)

func TestToken(t *testing.T) {
	tok := Token()
	if len(tok) != 16 {
		t.Error("Token is not 34 characters long ", tok)
	}
	if string(tok[0]) != "1" {
		t.Error("Token does not have the right ver ", tok)
	}
}

func TestToken_ValidTime(t *testing.T) {
	tok := Token()
	if Valid(tok, time.Duration(100)) != true {
		t.Error("Token is not valid for at least 100 seconds")
	}
	if Valid(tok, time.Duration(100), time.Now().Add(time.Duration(101)*time.Second).Add(ClockSkew)) != false {
		t.Error("Token did not invalidate properly by Time")
	}
}

func TestToken_ValidTime2(t *testing.T) {
	tok := Token()
	if Valid(tok, time.Duration(100)) != true {
		t.Error("Token is not valid for at least 100 seconds")
	}
	expired_time := time.Now().Add(time.Duration(101) * time.Second).Add(ClockSkew)
	if Valid(tok, time.Duration(100), expired_time) {
		t.Error("Token did not invalidate properly by Time")
	}
}

func TestToken_ValidFormat(t *testing.T) {
	if Valid("", time.Duration(100)) {
		t.Error("Empty Token is found Valid")
	}
	if Valid("-abcdefghijabcdefghijabcdefghijabc", 100) {
		t.Error("random Token string is found Valid")
	}
}
