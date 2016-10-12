#!/usr/bin/env python3

import jira
import modecsvparse
import os
import psycopg2
import psycopg2.extras

jirauser = os.environ.get("JIRA_USER")
jirapass = os.environ.get("JIRA_PASS")
dbuser = os.environ.get("REDSHIFT_DB_USER")
dbpass = os.environ.get("REDSHIFT_DB_PASS")
dbdays = os.environ.get("DB_DAYS_INTERVAL")
cozmo_table = os.environ.get("COZMO_TABLE")

def db_connect(dbname="anki", ):
  try:
    conn = psycopg2.connect("dbname='anki' host='127.0.0.1' port='5439' user='%s' password='%s'" % (dbuser,dbpass))

  except:
    print("I am unable to connect to the database")
    return None

  cursor = conn.cursor(cursor_factory=psycopg2.extras.DictCursor)
  return cursor


def run_sql(sql):
  cursor = db_connect()
  if cursor:
    try:
      cursor.execute(sql)
    except:
      print("I am unable to access the schema for das")
      return None

    rows = cursor.fetchall()

    return rows
  else:
    return None


def jira_connect(url, username, password):
  return jira.JIRA(url, basic_auth=(username, password))


def parse_query(jconn):
  sql = "SELECT * FROM das.{} " \
        "WHERE event = 'RobotFirmware.CrashReport.Data' " \
        "AND written_at_ms >= (GETDATE() - INTERVAL '{} days')" \
        "ORDER BY ts DESC".format(cozmo_table, dbdays)

  rows = run_sql(sql)

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
    crash, registers = modecsvparse.parse_dump(typestr, row['data'])
    crash_dict = {row['apprun']: registers}
    crashes_dict.setdefault(crash, []).append(crash_dict)
    affects_versions_dict.setdefault(crash, []).append(row['app'])

  for crash,apprun_registers in crashes_dict.items():
    jissues = jconn.search_issues('summary ~ "Fix firmware crash {}" ORDER BY created DESC'.format(crash))

    if len(jissues) >= 1: # comment on existing issues
      if jissues[0].fields.status.name == 'Closed':
        try:
          jconn.transition_issue(jissues[0].key, 341)
          print("Re-opening ticket {}".format(jissues[0].key))
        except:
          print("Unable to re-open {}".format(jissues[0].key))

      for i in jissues:
        comment = build_comment(apprun_registers)

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

    else: # create new JIRA issue
      description = build_jira_description(apprun_registers)
      affects_versions = build_affects_verions_list(affects_versions_dict, crash)

      new_issue = jconn.create_issue(project='COZMO', summary='Fix firmware crash {}'.format(crash),
                                     description='{}'.format(description), issuetype={'name': 'Bug'},
                                     components=[{"name": "triage"}],versions=affects_versions)
      print("Created new issue: {}".format(new_issue.key))

def build_comment(apprun_registers):
  comment = "{} new occurrences: \n".format(len(apprun_registers))
  for apprun in apprun_registers:
    for apprun_id, registers in apprun.items():
      comment += "{}\n".format(apprun_id)

  return comment


def build_affects_verions_list(affects_versions_dict, crash):
  affects_versions = []
  for version in set(affects_versions_dict[crash]):
    arr = str(version).split(".")
    version = "COZMO {}.{}.{}".format(arr[0], arr[1], arr[2])
    affects_versions.append({"name": version})

  return affects_versions


def build_jira_description(apprun_registers):
  description = ""
  for apprun_id, registers in apprun_registers[0].items():
    description += "Registers for {}:\n".format(apprun_id)
    for r in registers:
      description += "\t{}\t: {:08x}\n".format(r, registers[r])

  description += "\n{} occurrences: \n".format(len(apprun_registers))
  for apprun in apprun_registers:
    for apprun_id, registers in apprun.items():
      description += "{}\n".format(apprun_id)

  return description


if __name__=="__main__":
  jconn = jira_connect("https://ankiinc.atlassian.net", jirauser, jirapass)
  parse_query(jconn=jconn)
