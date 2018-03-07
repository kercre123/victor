#!/usr/bin/env python3
# encoding: utf-8

import argparse
import boto3
from boto3.s3.transfer import S3Transfer
import botocore
import datetime
import errno
import io
import json
import logging
import os
import re
import requests
import shutil
import sys
import textwrap
import time
import verify_codelab_file

# set up default logger
Logger = logging.getLogger('codelab.verify_submissions')

############### GLOBAL ENVS ###############
try:
  ACCESS_KEY = os.environ['ACCESS_KEY']
except KeyError:
  Logger.error('Please set the environment variable ACCESS_KEY')
  sys.exit(1)

try:
  ACCESS_SECRET_KEY = os.environ['ACCESS_SECRET_KEY']
except KeyError:
  Logger.error('Please set the environment variable ACCESS_SECRET_KEY')
  sys.exit(1)

try:
  AWS_REGION = os.environ['AWS_REGION']
except KeyError:
  Logger.error('Please set the environment variable AWS_REGION')
  sys.exit(1)

try:
  S3_BUCKET = os.environ['S3_BUCKET']
except KeyError:
  Logger.error('Please set the environment variable S3_BUCKET')
  sys.exit(1)

try:
  SLACK_CHANNEL = os.environ['SLACK_CHANNEL']
except KeyError:
  Logger.error('Please set the environment variable SLACK_CHANNEL')
  sys.exit(1)

try:
  SLACK_TOKEN = os.environ['SLACK_TOKEN']
except KeyError:
  Logger.error('Please set the environment variable SLACK_TOKEN')
  sys.exit(1)

try:
  SLACK_TOKEN_URL = os.environ['SLACK_TOKEN_URL']
except KeyError:
  Logger.error('Please set the environment variable SLACK_TOKEN_URL')
  sys.exit(1)


def get_unverified_codelab_filelist():
  unverified_codelab_file_pattern = r'(^production/[a-zA-Z\d]{23}/[a-fA-F\d]{26}/[a-fA-F\d]{26}_file.codelab$)'
  unverified_codelab_file_pattern = re.compile(unverified_codelab_file_pattern, re.IGNORECASE)

  contents = None
  try:
    client = boto3.client('s3',
                          AWS_REGION,
                          aws_access_key_id=ACCESS_KEY,
                          aws_secret_access_key=ACCESS_SECRET_KEY)
    contents = client.list_objects(Bucket=S3_BUCKET)['Contents']

  except Exception as e:
    Logger.error('There was an exception listing S3 contents:\n{}'.format(e))

  codelab_files = []
  for key in contents:
    matches = re.search(unverified_codelab_file_pattern, str(key['Key']))
    if bool(matches):
      codelab_files.append(key['Key'])

  return codelab_files


def verify_codelab_files(args):
  codelab_files = get_unverified_codelab_filelist()

  codelab_file_status_dict = {}
  log_capture_string = None
  for codelab_file in codelab_files:
    codelab_filepath, codelab_json_filepath = download_codelab_file_and_json(args, codelab_file)

    ### Setup the console handler with a StringIO object
    logger = logging.getLogger('codelab.verify_file')
    log_capture_string = io.StringIO()
    ch = logging.StreamHandler(log_capture_string)
    ch.setLevel(logging.ERROR)
    logger.addHandler(ch)

    if verify_codelab_file.is_valid_codelab_file(codelab_filepath):

      with open(codelab_json_filepath, encoding='utf8') as data:
        project_metadata = json.load(data)

      codelab_file_status_dict[codelab_file] = {'status': 'good', 'project_name': project_metadata['name']}

      rename_files_to_human_readable(args, codelab_filepath, codelab_json_filepath, project_metadata)
    else:
      log_contents = log_capture_string.getvalue()
      codelab_file_status_dict[codelab_file] = {"status": "bad", "error_msg": log_contents}

  try:
    log_capture_string.close()
  except:
    Logger.warning('No io.StringIO() to close.')

  for codelab_file, status in codelab_file_status_dict.items():
    if status['status'] is 'bad':
      error_log_filename = os.path.join(args.output_dir, os.path.dirname(codelab_file), 'error.log')

      with open(error_log_filename, 'w') as file_error_log:
        file_error_log.write(status['error_msg'])

      new_s3_filepath = archive_s3_files(codelab_file, status['status'])

      upload_error_log_to_s3(error_log_filename, new_s3_filepath)

    else:
      archive_s3_files(codelab_file, status['status'])

  try:
    shutil.rmtree(os.path.join(args.output_dir, 'production'))
  except Exception as e:
    Logger.warning('Could not remove bad codelab files from filesystem:\n{}'.format(e))

  good = len([file for (file, status) in codelab_file_status_dict.items() if status['status'] is 'good'])
  if good > 1:
    zipfile = make_zip_archive(args)
    post_results_to_slack('There were {} valid codelab submissions since I last checked.'.format(good))
    post_file_to_slack(zipfile)

  else:
    post_results_to_slack('There were no valid codelab submissions since I last checked :cry:')

  try:
    shutil.rmtree(args.output_dir)
  except:
    Logger.warning('Could not remove left over build files.')


####################################
############# HELPERS ##############
####################################
def make_zip_archive(args):
  date = datetime.datetime.now()
  filename = 'codelab-validated-projects-{}'.format(date.strftime("%Y%m%d-%H%M%S"))
  build_dir = os.path.dirname(args.output_dir)
  zipfile = shutil.make_archive(os.path.join(build_dir, filename), 'zip', args.output_dir)
  return zipfile


def archive_s3_files(codelab_file, status):
  s3_parent_key = os.path.dirname(codelab_file)
  new_s3_parent_key = s3_parent_key.replace('production', 'production/{}'.format(status))
  try:
    s3 = boto3.resource('s3',
                        AWS_REGION,
                        aws_access_key_id=ACCESS_KEY,
                        aws_secret_access_key=ACCESS_SECRET_KEY)

    items_in_dir = s3.meta.client.list_objects(Bucket=S3_BUCKET, Prefix=s3_parent_key)

    for files in items_in_dir['Contents']:
      for k,v in files.items():
        if k == 'Key':
          new_filename = v.replace('production', 'production/{}'.format(status))
          s3.Object(S3_BUCKET, new_filename).copy_from(CopySource='{}/{}'.format(S3_BUCKET, v))
          s3.Object(S3_BUCKET, v).delete()

  except Exception as e:
    Logger.error('Cannot move files to archive paths on s3:\n{}\n{}\n{}'.format(e, codelab_file, status))

  return new_s3_parent_key


def upload_error_log_to_s3(error_log_filename, s3_filepath):
  try:
    transfer = S3Transfer(boto3.client('s3',
                                       AWS_REGION,
                                       aws_access_key_id=ACCESS_KEY,
                                       aws_secret_access_key=ACCESS_SECRET_KEY))
    transfer.upload_file(error_log_filename, S3_BUCKET, os.path.join(s3_filepath, 'error.log'))

  except Exception as e:
      Logger.error('Cannot upload error log to s3:\n{}\n{}\n{}'.format(e, error_log_filename, s3_filepath))


def rename_files_to_human_readable(args, codelab_filepath, codelab_json_filepath, project_metadata):
  time_to_miliseconds = 1000
  submission_date = time.strftime('%Y%m%d-%H%M%S', time.gmtime(project_metadata['date'] / time_to_miliseconds))
  shutil.move(codelab_json_filepath,
              os.path.join(args.output_dir, '{}-{}.json'.format(project_metadata['name'], submission_date)))
  shutil.move(codelab_filepath,
              os.path.join(args.output_dir, '{}-{}.codelab'.format(project_metadata['name'], submission_date)))


def download_codelab_file_and_json(args, codelab_file):
  makedirs_exist_ok('{}/{}'.format(args.output_dir, os.path.dirname(codelab_file)))
  codelab_json_file = str(codelab_file).replace('_file.codelab', '_metadata.json')
  codelab_filepath = os.path.join(args.output_dir, codelab_file)
  codelab_json_filepath = os.path.join(args.output_dir, codelab_json_file)
  try:
    s3 = boto3.resource('s3',
                        AWS_REGION,
                        aws_access_key_id=ACCESS_KEY,
                        aws_secret_access_key=ACCESS_SECRET_KEY)
    s3.Bucket(S3_BUCKET).download_file(codelab_file, codelab_filepath)
    s3.Bucket(S3_BUCKET).download_file(codelab_json_file, codelab_json_filepath)

  except botocore.exceptions.ClientError as e:
    if e.response['Error']['Code'] == "404":
      Logger.error('The s3 object does not exist:\n{}\n{}'.format(codelab_file, codelab_json_file))
    else:
      Logger.error('Cannot download s3 object:\n{}\n{}\n{}'.format(e, codelab_file, codelab_json_file))

  return codelab_filepath, codelab_json_filepath


def makedirs_exist_ok(path):
  try:
    os.makedirs(path)
  except OSError as exception:
    if exception.errno != errno.EEXIST:
      raise


def post_results_to_slack(message):
  payload = '{{\
              "text": "{0}\n",\
              "mrkdwn": true,\
              "channel": "{1}",\
              "username": "buildbot",\
          }}'.format(message, SLACK_CHANNEL)

  headers = { 'content-type': "application/json" }
  response = requests.request("POST", SLACK_TOKEN_URL, data=payload, headers=headers)


def post_file_to_slack(file):
  payload = {
    'channels': SLACK_CHANNEL,
    'token': SLACK_TOKEN,
    'filename': os.path.basename(file),
    'filetype': 'zip'
  }
  file_to_send = { 'file': (os.path.basename(file), open(file, 'rb'), 'zip') }

  requests.post('https://slack.com/api/files.upload', params=payload, files=file_to_send)


####################################
############### MAIN ###############
####################################
def parse_args(argv=[], print_usage=False):
  parser = argparse.ArgumentParser(
    formatter_class=argparse.RawDescriptionHelpFormatter,
    description='Verifies Code Lab user submission projects.',
    epilog=textwrap.dedent('''
    options description:
      [localized strings directory]
    ''')
  )

  parser.add_argument('--debug', '-d', '--verbose', dest='debug', action='store_true',
                      help='prints debug output')
  parser.add_argument('--output_dir', action='store', default="../../../build/codelab_submissions",
                      help='Sound file output dir.')

  if print_usage:
    parser.print_help()
    sys.exit(2)

  args = parser.parse_args(argv)

  if (args.debug):
    print(args)
    Logger.setLevel(logging.DEBUG)
  else:
    Logger.setLevel(logging.INFO)

  return args


def run(args):
  verify_codelab_files(args)


if __name__ == '__main__':
  args = parse_args(sys.argv[1:])
  sys.exit(run(args))
