#!/usr/bin/env python3
# encoding: utf-8

import requests
import os
import argparse
import sys
import logging
import json

try:
    SLACK_TOKEN_URL = os.environ['SLACK_TOKEN_URL']
except KeyError:
    print("Please set the environment variable SLACK_TOKEN_URL")
    sys.exit(1)

try:
    SLACK_CHANNEL = os.environ['SLACK_CHANNEL']
except KeyError:
    print("Please set the environment variable SLACK_CHANNEL")
    sys.exit(1)


Logger = logging.getLogger('slack_messaging')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(levelname)s - %(message)s')
stdout_handler.setFormatter(formatter)
Logger.addHandler(stdout_handler)

SLACK_FILE_UPLOAD_URL="https://slack.com/api/files.upload"
SLACK_TITLE="Sent Message"

##############################################################################################################
### Post to Slack
##############################################################################################################
def post_results_to_slack(results_by_color, title):
    payload = '{{\
                    "text": "*{0}*\n",\
                    "mrkdwn": true,\
                    "channel": "{1}",\
                    "username": "buildbot",\
                    "attachments": ['.format(title, SLACK_CHANNEL)
    for color, message in results_by_color.items():
        payload += '{{\
                            "text": "{1}",\
                            "fallback": "{1}",\
                            "color": "{0}"\
                        }},'.format(color, message)
    payload += ']}}'
    Logger.debug(payload)
    headers = {
        'content-type': "application/json"
    }
    response = requests.request("POST", SLACK_TOKEN_URL, data=payload, headers=headers)
    Logger.debug(response.text)

def post_link_to_slack(link, title):
    payload = '{{\
                    "text": "*{0}*\n",\
                    "mrkdwn": true,\
                    "channel": "{1}",\
                    "username": "buildbot",\
                    "attachments": [ \
                    {{\
                        "title": "DAS Documentation",\
                        "title_link": "{2}", \
                        "fallback": "{2}",\
                    }}, \
                    ]}}'.format(title, SLACK_CHANNEL, link)
    Logger.debug(payload)
    headers = {
        'content-type': "application/json"
    }
    response = requests.request("POST", SLACK_TOKEN_URL, data=payload, headers=headers)
    Logger.debug(response.text)


def post_file_to_slack(file_name):
    payload = { 
                'channels' : SLACK_CHANNEL,
                'token'    : SLACK_TOKEN,
                'filename' : file_name,
                'filetype' : 'html'
              }
    file =    {
                'file'     : (file_name, open(file_name, 'rb'), 'html')
              }
    response = requests.post(SLACK_FILE_UPLOAD_URL, params=payload, files=file)
    Logger.debug(response.text)

##############################################################################################################
### Main
##############################################################################################################

def parse_arguments(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('--debug', '-d', '--verbose', dest='debug', action='store_true',
                        help='prints debug output with various directory names and all values aggregated')
    parser.add_argument('-f', '--file',action='store',
                        help="File to attach to message")
    parser.add_argument('-m', '--message',action='store',
                        help="Message which will be shown")
    parser.add_argument('-t', '--title',action='store',
                        help="Message which will be shown")
    parser.add_argument('-l', '--link',action='store',
                        help="Message which will be shown")
    options = parser.parse_args(args)


    if (options.debug):
        print(options)
        Logger.setLevel(logging.DEBUG)
    else:
        Logger.setLevel(logging.INFO)

    return (parser, options)


if __name__ == '__main__':
    args = sys.argv[1:]
    parser, options = parse_arguments(args)

    if (options.file):
        post_file_to_slack(options.file)

    if (options.message):
        title = SLACK_TITLE
        if (options.title):
            title = options.title
        post_results_to_slack(json.loads(options.message), title)

    if (options.link):
        title = SLACK_TITLE
        if (options.title):
            title = options.title
        post_link_to_slack(options.link, title)
