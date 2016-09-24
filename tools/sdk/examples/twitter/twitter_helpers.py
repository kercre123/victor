# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Twitter helper functions

Wrapper functions for integrating Cozmo with Twitter using Tweepy.
'''

from io import BytesIO
import json
import sys
import cozmo
try:
    import tweepy
except ImportError:
    sys.exit("Cannot import tweepy: Do `pip3 install tweepy` to install")


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

    def upload_images(self, images, use_jpeg=False, jpeg_quality=70):
        '''
        Args:
            images (list of PIL.Image): images to upload
            use_jpeg (bool):
            jpeg_quality (int): quality used for compressing jpegs (if use_jpeg == True)

        Returns:
            list of media_id
        '''

        media_ids = []
        for image in images:
            img_io = BytesIO()

            if use_jpeg:
                image.save(img_io, 'JPEG', quality=jpeg_quality)
                filename = "temp.jpeg"
                img_io.seek(0)
            else:
                image.save(img_io, 'PNG')
                filename = "temp.png"
                img_io.seek(0)

            upload_res = self.twitter_api.media_upload(filename, file=img_io)
            media_ids.append(upload_res.media_id)

        return media_ids

    def post_tweet(self, tweet_text, reply_id=None, media_ids=None):
        ''''post a tweet to the timeline, trims tweet if appropriate
            reply_id is optional, nests the tweet as reply to that tweet (use id_str element from a tweet)
        '''
        tweet_text = self.trim_tweet_text(tweet_text)
        try:
            self.twitter_api.update_status(tweet_text, reply_id, media_ids=media_ids)
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
        from_user = json_data.get('user')
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

def has_default_twitter_keys(twitter_keys):
    default_key = 'XXXXXXXXXX'
    return (twitter_keys.CONSUMER_KEY == default_key) and (twitter_keys.CONSUMER_SECRET == default_key) and \
           (twitter_keys.ACCESS_TOKEN == default_key) and (twitter_keys.ACCESS_TOKEN_SECRET == default_key)

def auth_twitter(twitter_keys):
    '''Perform OAuth authentication with twitter, using the keys provided'''
    if has_default_twitter_keys(twitter_keys):
        cozmo.logger.error("You need to configure your twitter_keys")

    auth = tweepy.OAuthHandler(twitter_keys.CONSUMER_KEY, twitter_keys.CONSUMER_SECRET)
    auth.set_ACCESS_TOKEN(twitter_keys.ACCESS_TOKEN, twitter_keys.ACCESS_TOKEN_SECRET)
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
