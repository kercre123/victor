package token

import (
	"anki/config"
	"anki/log"
	"time"

	"github.com/anki/sai-go-util/testutils/testtime"
	"github.com/aws/aws-sdk-go/aws/credentials"
)

var testableTime = testtime.New()

// TokenRefreshWindow indicates how soon before actual expiration the STS token will be considered expired
const TokenRefreshWindow = time.Hour

type stsCredentialsCache struct {
	expiration  time.Time
	credentials *credentials.Credentials
}

func (c *stsCredentialsCache) add(expiration string, credentials *credentials.Credentials) {
	expirationTime, err := time.Parse(time.RFC3339, expiration)
	if err != nil {
		log.Printf("Error parsing StsToken exiration timestamp timestamp %q: %v\n", expiration, err)
		return
	}

	c.expiration = expirationTime
	c.credentials = credentials
}

func (c *stsCredentialsCache) expired() bool {
	if c.credentials == nil {
		return true
	}

	// refresh token in case it expires within "TokenRefreshWindow"
	return testableTime.Now().UTC().After(c.expiration.Add(-TokenRefreshWindow))
}

var stsCache stsCredentialsCache

// Currently STS tokens are only refreshed on demand since they are rarely
// used. The only current use (uploading of log files) is not time critical.
// In the future we may implement a pro-active refreshing mechanism (similar
// to the one in refresher.go).
func getStsCredentials() (*credentials.Credentials, error) {
	if !stsCache.expired() && stsCache.credentials != nil {
		return stsCache.credentials, nil
	}

	perRPCCreds, err := GetAccessor().Credentials()
	if err != nil {
		return nil, err
	}

	client, err := newConn(config.Env.Token, perRPCCreds)
	if err != nil {
		return nil, err
	}
	defer client.Close()

	bundle, err := client.refreshStsCredentials()
	if err != nil {
		return nil, err
	}

	stsToken := bundle.GetStsToken()

	awsCredentials := credentials.NewStaticCredentialsFromCreds(credentials.Value{
		AccessKeyID:     stsToken.AccessKeyId,
		SecretAccessKey: stsToken.SecretAccessKey,
		SessionToken:    stsToken.SessionToken,
	})

	stsCache.add(stsToken.GetExpiration(), awsCredentials)

	return awsCredentials, nil
}
