#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import cozmo
import twitter_helpers
import user_twitter_keys as twitter_keys


'''Example for integrating Cozmo with Twitter
   Cozmo will read aloud each new tweet as it appears on your twitter stream
   See user_twitter_keys.py for details on how to setup a twitter account and get access keys
'''


class CozmoReadsTweetsStreamListener(twitter_helpers.CozmoTweetStreamListener):
    '''React to Tweets sent to our Cozmo, live, as they happen...'''

    def __init__(self, coz, twitter_api):
        super().__init__(coz, twitter_api)

    def on_tweet_from_user(self, json_data, tweet_text, from_user, is_retweet):
        '''Called on every tweet that appears in the stream'''

        user_name = from_user.get('screen_name')
        if is_retweet:
            # Remove the redundant RT string at the start of retweets
            rt_prefix = "RT "
            rt_loc = tweet_text.find(rt_prefix)
            if rt_loc >= 0:
                tweet_text = tweet_text[rt_loc+len(rt_prefix):]
            text_to_say = user_name + " retweeted " + tweet_text
        else:
            text_to_say = user_name + " tweeted " + tweet_text

        text_to_say = text_to_say.strip()

        cozmo.logger.info('Cozmo says: "' + text_to_say + '"')

        self.cozmo.say_text(text_to_say).wait_for_completed()


def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    twitter_api, twitter_auth = twitter_helpers.init_twitter(twitter_keys)
    stream_listener = CozmoReadsTweetsStreamListener(coz, twitter_api)
    twitter_stream = twitter_helpers.CozmoStream(twitter_auth, stream_listener)
    twitter_stream.userstream()


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)

