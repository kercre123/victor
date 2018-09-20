// +build !vicos

package jwt

func tokenPath() string {
	return "/tmp/victoken"
}
