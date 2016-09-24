# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
''' User's Twitter keys

Secret Twitter keys for the user that you want cozmo_reads_tweets.py to work with
   To generate these you need to setup a Twitter account with developer keys:
   1) Log into your Twitter account in your web browser (or create a new account if you prefer)
   2) Go to https://apps.twitter.com/app/new and create your application:
      a) Fill in the name and details etc. (most are optional)
      b) Select "Permissions" tab and set Access to Read only (this example only needs to read your tweets)
      c) Select "Keys and Access Tokens" tab and click "Generate an Access Token and Secret"
      d) Paste your consumer key + secret, and access token + secret below into the XXXXXXXXXX fields
      e) Keep this file safe - don't distribute it to other people, these keys allow anyone full access to the
         associated Twitter account!!!
'''

# Secret keys for doing OAuth with Twitter, these should be kept private...
# Keep the "Consumer Secret" a secret. This key should never be human-readable in your application...
# This access token can be used to make API requests on that account's behalf. Do not share your access token secret with anyone.

# DO NOT DISTRIBUTE THIS FILE WITH YOUR KEYS AND SECRETS IN - THEY GIVE FULL ACCESS TO YOUR TWITTER ACCOUNT

CONSUMER_KEY = 'XXXXXXXXXX'
CONSUMER_SECRET = 'XXXXXXXXXX'
ACCESS_TOKEN = 'XXXXXXXXXX'
ACCESS_TOKEN_SECRET = 'XXXXXXXXXX'
