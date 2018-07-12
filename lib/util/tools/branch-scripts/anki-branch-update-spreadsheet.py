#!/usr/bin/env python3
#
# GIT list processing is taken from anki git-log-cpr.py
# Google Sheets API stuff is taken from Google Sheets API sample app
#
# https://developers.google.com/sheets/api/quickstart/python
#
# Prerequisites:
#  brew install python3
#  pip3 install google-api-python-client
#
from __future__ import print_function

# Google API authentication scope
# If modifying these scopes, delete your previously saved credential file
# from ~/.credentials/{CREDENTIAL_FILE}
#
APPLICATION_NAME = 'Anki Branch Spreadsheet Update'
APPLICATION_SCOPE = 'https://www.googleapis.com/auth/spreadsheets'
CLIENT_SECRET_FILE = 'client_secret.json'
CREDENTIAL_FILE = 'sheets.googleapis.com-anki-branch-update-spreadsheet.json'

import httplib2
import os
import argparse
import re
import subprocess
import sys

from apiclient import discovery
from oauth2client import client
from oauth2client import tools
from oauth2client.file import Storage

# Which range of this sheet contains known commits?
# Assume sheet starts header row, blank row, then data.
# The blank row is needed so new data can be added without header formatting.
def get_select_range(options):
    return '{}!A3:A'.format(options.sheet)

# Where do we want to append new entries?  Sheets API will start at this cell,
# then scan down looking for an open row.  News rows will be added ahead of
# the open row, ie. at the bottom of the first section in the sheet.
def get_append_range(options):
    return '{}!A3'.format(options.sheet)

def parse_args(args):
    class DefaultHelpParser(argparse.ArgumentParser):
        def error(self, message):
            sys.stderr.write('error: %s\n' % message)
            self.print_help()
            sys.exit(2)

    parser = DefaultHelpParser(parents=[tools.argparser], description="List commits appearing in master but not release branch")

    parser.add_argument('spreadsheetId',
                    action='store',
                    help="Google Spreadsheet Document ID")

    parser.add_argument('sheet',
                    action='store',
                    help="Google Spreadsheet Sheet Name")

    parser.add_argument('master_branch',
                    action='store',
                    default='master',
                    help="git branch containing source commits")

    parser.add_argument('release_branch',
                    action='store',
                    default='release/candidate',
                    help="git branch where commits will be cherry-picked")

    parser.add_argument('-x', '--exclude-file', action='store',
                        help="Path to file listing commits to exclude from list. One commit per line starting with sha1")

    parser.add_argument('--exclude-url', action='store',
                        help="URL to list of commits to exclude from list. One commit per line starting with sha1")



    options = parser.parse_args(args)
    return options

def git_sync_branch(branch):
    cmd = [
        'git', 'checkout', branch
    ]
    subprocess.call(cmd)
    cmd = [
        'git', 'pull'
    ]
    subprocess.call(cmd)
    cmd = [
        'git', 'submodule', 'update', '--init', '--recursive'
    ]
    subprocess.call(cmd)

def git_sync_branches(master_branch, release_branch):
  cmd = [
    'git', 'branch', '--no-color'
  ]
  current_branch = ''
  lines = subprocess.check_output(cmd)
  lines = lines.decode()
  lines = lines.split('\n')
  for line in lines:
    if (line.startswith('*')):
      current_branch = line.split(' ')[1]
  git_sync_branch(master_branch)
  git_sync_branch(release_branch)
  if release_branch != current_branch:
    cmd = [
      'git', 'checkout', current_branch
    ]
    subprocess.call(cmd)

def git_compare_branches(master_branch, release_branch):
  ref_spec = "%s...%s" % (options.master_branch, options.release_branch)
  cmd = [
    'git', 'log',
    '--cherry-pick',
    '--oneline',
    '--no-merges',
    '--left-only',
    ref_spec
  ]
  output = subprocess.check_output(cmd)
  output = output.decode()
  output = output.split('\n')

  lines = []
  for line in output:
    line = line.strip()
    if line:
      lines.append(line)

  return lines

def filter_commits(commits, exclude_list=[]):

    excluded_commits = [ c[0:7] for c in exclude_list ]

    remaining_commits = []
    for commit_info in commits:
        sha1 = commit_info[0:7]
        msg = commit_info[8:]
        if sha1 not in excluded_commits:
            remaining_commits.append(commit_info)

    return remaining_commits

def fetch_excludes(excludes_url):
    import urllib2
    response = urllib2.urlopen(excludes_url)
    html = response.read()
    return html

def parse_excludes(excludes_content):
    exclude_list = []

    if not excludes_content:
        return exclude_list

    for raw_line in excludes_content.split("\n"):
        line = raw_line.strip()
        if re.search('^\#', line) or len(line) == 0:
            continue
        exclude_list.append(line)

    return exclude_list

def get_sheet_credentials(options):
  """Gets valid user credentials from storage.

  If nothing has been stored, or if the stored credentials are invalid,
  the OAuth2 flow is completed to obtain the new credentials.

  Returns:
    Credentials, the obtained credential.
  """
  home_dir = os.path.expanduser('~')
  credential_dir = os.path.join(home_dir, '.credentials')
  if not os.path.exists(credential_dir):
    os.makedirs(credential_dir)
  credential_path = os.path.join(credential_dir, CREDENTIAL_FILE)

  store = Storage(credential_path)
  credentials = store.get()
  if not credentials or credentials.invalid:
    flow = client.flow_from_clientsecrets(CLIENT_SECRET_FILE, APPLICATION_SCOPE)
    flow.user_agent = APPLICATION_NAME
    credentials = tools.run_flow(flow, store, options)
    print('Storing credentials to ' + credential_path)
  return credentials

def get_sheet_values(options):
  credentials = get_sheet_credentials(options)
  http = credentials.authorize(httplib2.Http())
  discoveryUrl = ('https://sheets.googleapis.com/$discovery/rest?'
    'version=v4')
  service = discovery.build('sheets', 'v4', http=http,
    discoveryServiceUrl=discoveryUrl)
  spreadsheets = service.spreadsheets()
  values = spreadsheets.values()
  return values

def get_sheet_commits(options):
  id = options.spreadsheetId
  range = get_select_range(options)

  values = get_sheet_values(options)
  result = values.get(spreadsheetId=id, range=range).execute()
  rows = result.get('values', [])

  commits = []
  for row in rows:
    if not row:
      continue
    commit = row[0].strip()
    if not commit:
      continue
    commits.append(commit)
  return commits

def pretty_ticket(ticket):
  url = 'https://ankiinc.atlassian.net/browse/{}'.format(ticket)
  link = '=HYPERLINK("{}", "{}")'.format(url, ticket)
  return link

# List of key:pattern pairs used to match JIRA project names
PATTERNS = {
  'COZMO': '^[Cc][Oo][Zz][Mm][Oo][- ]?(\d+)[\w:-]*(.+)',
  'OD': '^[Od][Dd][- ]?(\d+)[\w:-]*(.+)',
  'BI': '^[Bb][Ii][- ]?(\d+)[\w:-]*(.+)',
  'VIC': '^[Vv][Ii][Cc][- ]?(\d+)[\w:-]*(.+)',
  'CLAI': '^[Cc][Ll][Aa][Ii][- ]?(\d+)[\w:-]*(.+)',
  'SAI': '^[Ss][Aa][Ii][- ]?(\d+)[\w:-]*(.+)'
}

def append_sheet_commits(options, commits):
  newrows = []
  for commit in commits:
    commit = commit.strip()
    if not commit:
      continue

    # Split 'id text' into fields
    match = re.search('([0-9a-f]+)(.+)', commit)
    if not match:
      continue

    commit = match.group(1).strip()
    text = match.group(2).strip()
    ticket = ''

    # Attempt to match on each project name
    for key in PATTERNS:
      match = re.search(PATTERNS[key], text)
      if match:
        ticket = key + '-' + match.group(1).strip()
        text = match.group(2).strip()
        break

    # Format ticket
    if ticket:
      ticket = pretty_ticket(ticket)

    # Format new data row
    newrows.append([commit, ticket, text])

  # Add new rows to spreadsheet.  Newest rows appear at top of list,
  # so list is reversed to put newest rows at bottom of spreadsheet.
  #
  if newrows:
    newrows.reverse()
    spreadsheetId = options.spreadsheetId
    appendRange = get_append_range(options)
    values = get_sheet_values(options)
    body = { "values": newrows }
    request = values.append(spreadsheetId=spreadsheetId, range=appendRange,
      insertDataOption='INSERT_ROWS',
      valueInputOption='USER_ENTERED',
      body=body)
    response = request.execute()

  return newrows

def main(options):

    # Sync branches
    git_sync_branches(options.master_branch, options.release_branch)

    # Compare branches
    commits = git_compare_branches(options.master_branch, options.release_branch)
    # Apply excludes
    exclude_list = []

    if options.exclude_file and os.path.exists(options.exclude_file):
        with open(options.exclude_file, 'r') as f:
            contents = f.read()
            xlist = parse_excludes(contents)
            exclude_list.extend(xlist)

    if options.exclude_url:
        contents = fetch_excludes(options.exclude_url)
        if (contents):
            xlist = parse_excludes(contents)
            exclude_list.extend(xlist)

    commits = filter_commits(commits, exclude_list)

    print('Found {} unmerged commits'.format(len(commits)))
    # for commit in commits:
    #   print(commit)

    # Fetch known commits from spreadsheet
    sheet_commits = get_sheet_commits(options)

    # Exclude known commits
    commits = filter_commits(commits, sheet_commits)

    print('Found {} commits missing from spreadsheet'.format(len(commits)))
    #for commit in commits:
    #  print(commit)

    # Append new commits to spreadsheet
    newrows = append_sheet_commits(options, commits)

    print('Added {} new rows to spreadsheet'.format(len(newrows)))
    for newrow in newrows:
      print(newrow)

    return 0

if __name__ == '__main__':
    options = parse_args(sys.argv[1:])
    #print(options)

    r = main(options)
    sys.exit(r)
