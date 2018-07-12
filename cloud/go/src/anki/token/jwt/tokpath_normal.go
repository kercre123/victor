// +build !linux

package jwt

func tokenPath() string {
	return "/tmp/victoken"
}
