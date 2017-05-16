#!/usr/bin/env python3

import codecs
import crashdump
import csv
from distutils.version import StrictVersion
import jira
import modecsvparse
import os
import requests
import sys
import time
import traceback

jirauser = os.environ.get("JIRA_USER")
jirapass = os.environ.get("JIRA_PASS")
mode_user = os.environ.get("MODE_USER")
mode_pass = os.environ.get("MODE_PASS")
mode_run_now_url = os.environ.get("MODE_RUN_NOW_URL")
mode_timeout = int(os.environ.get("MODE_TIMEOUT"))
db_days = os.environ.get("DB_DAYS_INTERVAL")
cozmo_table = os.environ.get("COZMO_TABLE")
production_version = os.environ.get("PROD_VERSION") # e.g. "1.0.1"


def jira_connect(url, username, password):
  return jira.JIRA(url, basic_auth=(username, password))


def parse_query(jconn):
  '''
  "curl -D- -u token:pass -X GET https://modeanalytics.com/anki/reports/94fe9480af0a?run=now"
  'from GET --> https://modeanalytics.com/anki/reports/94fe9480af0a/runs/???'
  "curl -D- -u token:pass -X GET https://modeanalytics.com/api/anki/reports/94fe9480af0a/runs/???/results/content.csv"
  '''

  print(">> Querying das.{} for firmware crashes - ".format(cozmo_table) + time.strftime("%c"))
  run_now_url = mode_run_now_url.replace("DAYS", db_days) \
                                .replace("MESSAGE_TABLE", cozmo_table)
  response = requests.get(run_now_url, auth=(mode_user, mode_pass))
  data = response.text
  report_url = data[data.find('https://modeanalytics.com/anki/reports'):data.find('">\n    <meta name="og:title')]
  report_csv = '{}/results/content.csv'.format(report_url.replace('anki', 'api/anki'))

  timeout_counter = 0
  while timeout_counter < mode_timeout:
    response = requests.get(report_csv, stream=True, auth=(mode_user, mode_pass))

    if 'run has not completed' in response.text:
      time.sleep(1)
      timeout_counter += 1
    else:
      break

  if timeout_counter == mode_timeout:
    print('>>> ERROR - mode report timeout reached:\n{}'.format(sys.stderr))
    sys.exit(1)

  rows = csv.DictReader(codecs.iterdecode(response.iter_lines(), 'utf-8'), delimiter=',')
  crashes_dict = {}
  affects_versions_dict = {}
  for row in rows:
    if row['s_val'] is None:
      continue
    print(row)
    try:
      summary = row['data'][0:16]
    except TypeError:
      summary = ""
    print("Parsing ", row['s_val'], summary)
    typestr = modecsvparse.extract_type(row['s_val'])
    try:
      crash, registers = modecsvparse.parse_dump(typestr, row['data'])
      crash_dict = {row['apprun']: {typestr: registers}}
      crashes_dict.setdefault(crash, []).append(crash_dict)
      affects_versions_dict.setdefault(crash, []).append(row['app'])
    except:
      print("Exception in parse_dump:")
      print('-' * 60)
      traceback.print_exc(file=sys.stdout)
      print('-' * 60)
      pass

  for crash,apprun_typestr_registers in crashes_dict.items():
    jissues = jconn.search_issues('summary ~ "Fix firmware crash {}" ORDER BY created DESC'.format(crash))

    if len(jissues) >= 1: # comment on existing issues
      if jissues[0].fields.status.name == 'Closed' and newer_version(crash, affects_versions_dict):
        try:
          jconn.transition_issue(jissues[0].key, 341)
          print("Re-opening ticket {}".format(jissues[0].key))
        except:
          print("Unable to re-open {}".format(jissues[0].key))

      for i in jissues:
        comment = build_comment(apprun_typestr_registers)

        try:
          jconn.add_comment(i.key, comment)
          print("Added comment to {}".format(i.key))
        except:
          print("Unable to add comment on {}".format(i.key))

        affects_versions = build_affects_verions_list(affects_versions_dict, crash)

        try:
          i.update(fields={'versions': affects_versions})
        except:
          print("Unable to add Affects Version to {}".format(i.key))

        try:
          i.update(fields={'components': [{"name": "crashes"}]})
        except:
          print("Unable to add crash component to {}".format(i.key))

    else: # create new JIRA issue
      description = build_jira_description(apprun_typestr_registers)
      affects_versions = build_affects_verions_list(affects_versions_dict, crash)

      new_issue = jconn.create_issue(project='COZMO',
                                     summary='Fix firmware crash {}'.format(crash),
                                     description='{}'.format(description),
                                     issuetype={'name': 'Bug'},
                                     components=[{"name": "triage"},{"name": "crashes"}])
      try:
        new_issue.update(fields={'versions': affects_versions})
      except:
        print("Unable to add Affects Version to {}".format(new_issue.key))

      print("Created new issue: {}".format(new_issue.key))

def build_comment(apprun_typestr_registers):
  comment = "{} new occurrences: \n".format(len(apprun_typestr_registers))
  for apprun in apprun_typestr_registers:
    for apprun_id, typestr_registers in apprun.items():
      comment += "{}\n".format(apprun_id)

  return comment


def newer_version(crash, affects_versions_dict):
  version = affects_versions_dict[crash][0].split('.')
  version = StrictVersion("{}.{}.{}".format(version[0],version[1],version[2]))
  prod_version = StrictVersion(production_version)
  if version > prod_version:
    return True
  else:
    return False


def build_affects_verions_list(affects_versions_dict, crash):
  affects_versions = []
  for version in set(affects_versions_dict[crash]):
    if version is not None:
      arr = str(version).split(".")
      version = "COZMO {}.{}.{}".format(arr[0], arr[1], arr[2])
      affects_versions.append({"name": version})

  return affects_versions


def build_jira_description(apprun_typestr_registers):
  description = ""
  for apprun_id, typestr_registers in apprun_typestr_registers[0].items():
    description += "Registers for {}:\n".format(apprun_id)

    for typestr, registers in typestr_registers.items():
      type_id = modecsvparse.TypeMap[typestr]

      for r in crashdump.RegisterMap[type_id]:
        description += "\t{}\t: {:08x}\n".format(r, registers[r])

  description += "\n{} occurrences: \n".format(len(apprun_typestr_registers))
  for apprun in apprun_typestr_registers:
    for apprun_id, registers in apprun.items():
      description += "{}\n".format(apprun_id)

  return description


if __name__=="__main__":
  jconn = jira_connect("https://ankiinc.atlassian.net", jirauser, jirapass)
  parse_query(jconn=jconn)
