package token

import (
	"anki/log"
	"anki/token/jwt"
	"anki/util"
	"clad/cloud"
	"context"
	"time"
)

func initRefresher(ctx context.Context) {
	go refreshRoutine(ctx)
}

func refreshRoutine(ctx context.Context) {
	for {
		const tokSleep = 5 * time.Minute
		const ntpSleep = 20 * time.Second

		// wait until we have a valid token
		var tok jwt.Token
		for {
			tok = jwt.GetToken()
			if tok != nil {
				break
			}
			log.Println("token refresher: no valid token yet, sleeping", tokSleep)
			if util.SleepSelect(tokSleep, ctx.Done()) {
				return
			}
		}

		// if robot thinks the token was issued in the future, we have the wrong time and
		// should wait for NTP to figure things out
		for time.Now().Before(tok.IssuedAt()) {
			if util.SleepSelect(ntpSleep, ctx.Done()) {
				return
			}
		}

		// now, time makes sense and we have a token - set a timer for when it should be refreshed
		// add 10s buffer so we're not TOO fast
		refreshDuration := tok.RefreshTime().Sub(time.Now()) + 10*time.Second
		if refreshDuration <= 0 {
			log.Println("token refresh: refreshing")
			ch := make(chan *response)
			queue <- request{m: cloud.NewTokenRequestWithJwt(&cloud.JwtRequest{}), ch: ch}
			msg := <-ch
			close(ch)
			if msg.err != nil {
				log.Println("Refresh routine error:", msg.err)
			}
			log.Println("token refresh: refresh done, sleeping", tokSleep)
			if util.SleepSelect(tokSleep, ctx.Done()) {
				return
			}
		} else {
			log.Println("token refresh: waiting for", refreshDuration)
			if util.SleepSelect(refreshDuration, ctx.Done()) {
				return
			}
		}
		// either...
		// - we just refreshed a new token
		// - we just waited long enough to get into the current token's refresh period
		// either way, we can just loop back around now and this routine will do the right thing (wait
		// a bunch more or start a refresh right away), while accounting for anything that changed in
		// the meantime (i.e. maybe external forces already forced a token refresh)
	}
}
