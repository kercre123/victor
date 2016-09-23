# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

"""Secret Twitter keys for your Cozmo robot's twitter account (used by tweet_at_cozmo.py)
   To generate these you need to setup a twitter account with developer keys:
   1) Create a twitter account for your Cozmo, and login to that account in your web browser
   2) Go to https://apps.twitter.com/app/new and create your application:
      a) Fill in the name and details etc. (most are optional)
      b) Select "Permissions" tab and set Access to Read and Write (this example needs to read and write tweets)
      c) Select "Keys and Access Tokens" tab and click "Generate an Access Token and Secret"
      d) Paste your consumer key + secret, and access token + secret below into the XXXXXXXXXX fields
      e) Keep this file safe - don't distribute it to other people, these keys allow anyone full access to the
         associated twitter account!!!
"""

# Secret keys for doing OAuth with Twitter, these should be kept private...
# Keep the "Consumer Secret" a secret. This key should never be human-readable in your application...
# This access token can be used to make API requests on that account's behalf. Do not share your access token secret with anyone.

# DO NOT DISTRIBUTE THIS FILE WITH YOUR KEYS AND SECRETS IN - THEY GIVE FULL ACCESS TO YOUR TWITTER ACCOUNT

consumer_key        = 'XXXXXXXXXX'
consumer_secret     = 'XXXXXXXXXX'
access_token        = 'XXXXXXXXXX'
access_token_secret = 'XXXXXXXXXX'

