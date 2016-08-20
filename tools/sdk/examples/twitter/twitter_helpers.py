import cozmo
import json
import sys
try:
    import tweepy
except ImportError:
    sys.exit("Cannot import tweepy: Do `pip3 install tweepy` to install")


'''Helpers for integrating Cozmo with Twitter using Tweepy'''


class CozmoTweetStreamListener(tweepy.StreamListener):
    '''Cozmo wrapper around tweepy.StreamListener
       Handles all data received from twitter stream.
    '''

    def __init__(self, coz, twitter_api):
        super().__init__(api=twitter_api)
        self.cozmo = coz
        self.twitter_api = twitter_api

    def trim_tweet_text(self, tweet_text):
        '''Trim a tweet to fit the Twitter max-tweet length'''
        max_tweet_length = 140
        concatenated_suffix = "..."
        if len(tweet_text) > max_tweet_length:
            tweet_text = tweet_text[0:(max_tweet_length-len(concatenated_suffix))] + concatenated_suffix
        return tweet_text

    def post_tweet(self, tweet_text, reply_id=None):
        ''''post a tweet to the timeline, trims tweet if appropriate
            reply_id is optional, nests the tweet as reply to that tweet (use id_str element from a tweet)
        '''
        tweet_text = self.trim_tweet_text(tweet_text)
        try:
            if reply_id:
                self.twitter_api.update_status(tweet_text, reply_id)
            else:
                self.twitter_api.update_status(tweet_text)
            return True
        except tweepy.error.TweepError as e:
            cozmo.logger.error("post_tweet Error: " + str(e))
            return False

    def on_tweet_from_user(self, json_data, tweet_text, from_user, is_retweet):
        ''''Called from on_data for anything that looks like a tweet
            Return False to stop stream and close connection.'''
        return True

    def on_non_tweet_data(self, json_data):
        ''''Called from on_data for anything that isn't a tweet
            Return False to stop stream and close connection.'''
        return True

    def on_data(self, raw_data):
        '''Called on all data (e.g. any twitter activity/action), including when we receive tweets
           Return False to stop stream and close connection.
        '''

        # parse data string into Json so we can inspect the contents
        json_data = json.loads(raw_data.strip())

        # is this a tweet?
        tweet_text = json_data.get('text')
        from_user  = json_data.get('user')
        is_retweet = json_data.get('retweeted')
        is_tweet = (tweet_text is not None) and (from_user is not None) and (is_retweet is not None)

        if is_tweet:
            return self.on_tweet_from_user(json_data, tweet_text, from_user, is_retweet)
        else:
            return self.on_non_tweet_data(json_data)


class CozmoStream(tweepy.Stream):
    '''Cozmo wrapper around tweepy.Stream
       Primarily just to avoid needing to import tweepy outside of this file
    '''


def auth_twitter(twitter_keys):
    '''Perform OAuth authentication with twitter, using the keys provided'''
    auth = tweepy.OAuthHandler(twitter_keys.consumer_key, twitter_keys.consumer_secret)
    auth.set_access_token(twitter_keys.access_token, twitter_keys.access_token_secret)
    return auth


def delete_all_tweets(twitter_api):
    '''Helper method to delete every tweet we ever made'''
    for status in tweepy.Cursor(twitter_api.user_timeline).items():
        try:
            twitter_api.destroy_status(status.id)
            cozmo.logger.info("Deleted Tweet " + str(status.id) +" = '" + status.text + "'")
        except Exception as e:
            cozmo.logger.info("Exception '" + str(e) + "' trying to Delete Tweet " + str(status.id) + " = '" + status.text + "'")


def init_twitter(twitter_keys):
    auth = auth_twitter(twitter_keys)
    twitter_api = tweepy.API(auth)
    return twitter_api, auth

