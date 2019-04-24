#!/usr/bin/env python3
# encoding: utf-8

import requests
import os
import argparse
import sys
import logging
import json
from bs4 import BeautifulSoup

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

SLACK_FILE_UPLOAD_URL = "https://slack.com/api/files.upload"
SLACK_TITLE           = "Sent Message"
NEW_DASMSG            = "group__das__msg__new"
REMOVE_DASMSG         = "group__das__msg__remove"
REMOVE_DASMSG_FILE    = "group__das__msg__remove.xml"
NEW_DASMSG_FILE       = "group__das__msg__new.xml"
GROUP_DASMSG          = "group__dasmsg"
HTML                  = "html"
XML                   = "xml"
MEMBERDEF             = "memberdef"
NEW_TITLE             = "NEW DAS Documentation"
REMOVED_TITLE         = "REMOVE DAS Documentation"
POST                  = "POST"
HEADERS               = {
                            'content-type': "application/json"
                        }
das_manager           = {"NEW DAS Documentation" :  "group__das__msg__new",
                         "REMOVE DAS Documentation" : "group__das__msg__remove"}

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
    response = requests.request(POST, SLACK_TOKEN_URL, data=payload, headers=HEADERS)
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


def read_file(file_path):
    content = ""
    with open(file_path, "r") as data_file:
        content = data_file.read()
    return content

def get_memberdefs(xml_content):
    memberdefs = []
    if xml_content:
        bs = BeautifulSoup(xml_content, XML)
        compounddef = bs.doxygen.compounddef
        if compounddef:
            memberdefs = compounddef.findAll(MEMBERDEF)
    return memberdefs

def is_memberdef_from_xml(xml_file_path):
    is_memberdef = False
    xml_content = read_file(xml_file_path)
    memberdefs = get_memberdefs(xml_content)
    if len(memberdefs) > 0:
        is_memberdef = True
    return is_memberdef

def generate_attachments(link, title):
    new_link = link.replace(GROUP_DASMSG, das_manager[title])
    attachment = '{{\
                        "title": "{0}",\
                        "title_link": "{1}", \
                        "fallback": "{1}"\
                 }}'.format(title, new_link)
    json_attachment = json.loads(attachment)
    return json_attachment

def build_payload(link, title, attachment=None, payload=None):
    if payload == None:
        payload = '{{\
                        "text": "*{0}*",\
                        "mrkdwn": true,\
                        "channel": "{1}",\
                        "username": "buildbot",\
                        "attachments": [\
                        {{\
                        "title": "DAS Documentation",\
                        "title_link": "{2}",\
                        "fallback": "{2}"\
                        }}\
                        ]\
                  }}'.format(title, SLACK_CHANNEL, link)

    json_payload = json.loads(payload)
    if attachment:
        json_payload["attachments"].append(attachment)

    return json.dumps(json_payload)

def post_link_to_slack(link, title):
    das_remove_file_path = "./{}/{}".format(XML, REMOVE_DASMSG_FILE)
    das_add_new_file_path = "./{}/{}".format(XML, NEW_DASMSG_FILE)
    is_das_removed = is_memberdef_from_xml(das_remove_file_path)
    is_das_added = is_memberdef_from_xml(das_add_new_file_path)

    payload = build_payload(link, title)
    if is_das_added:
        attachment = generate_attachments(link, NEW_TITLE)
        payload = build_payload(link=link, title=title, attachment=attachment, payload=payload)
    if is_das_removed:
        removed_attachment = generate_attachments(link, REMOVED_TITLE)
        payload = build_payload(link=link, title=title, attachment=removed_attachment, payload=payload)

    Logger.debug(payload)
    response = requests.request(POST, SLACK_TOKEN_URL, data=payload, headers=HEADERS)

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
