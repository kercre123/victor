#!/usr/bin/env python3
# encoding: utf-8

import requests
import os
import argparse
import sys
import fnmatch

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

RESULTS_FILE_NAME = "results.res"
OVERALL_RESULTS_FILE_NAME = "smoke_results.res"
RESULTS_FILE_PATTERN = "*_Results.log"

def process_result_file(file_path):
    result = ""
    is_fail = False
    file_name = os.path.splitext(os.path.basename(file_path))[0]
    with open(file_path, encoding="utf-8") as lines:
        for line in lines:
            if "False" in line:
                is_fail = True
    if not is_fail:
        result = "{} - PASSED".format(file_name)
    else:
        result = "{} - FAILED".format(file_name)
    result += "\n"
    return is_fail, result

def process_result_folder(folder_path):
    results_fail = ""
    results_pass = ""
    files = fnmatch.filter(os.listdir(folder_path), RESULTS_FILE_PATTERN)
    for file_result in files:
        abspath = "{}/{}".format(folder_path, file_result)
        is_fail, result = process_result_file(abspath)
        if is_fail:
            results_fail += result
        else:
            results_pass += result
    return results_pass, results_fail

def post_results_to_slack(results_pass, results_fail):
    payload = '{{\
                    "text": "*Nightly Cozmo Unity UI Automation Test Results:*\n",\
                    "mrkdwn": true,\
                    "channel": "{}",\
                    "username": "buildbot",\
                    "attachments": [\
                        {{\
                            "text": "{}",\
                            "fallback": "{}",\
                            "color": "danger"\
                        }},\
                        {{\
                            "text": "{}",\
                            "fallback": "{}",\
                            "color": "good"\
                        }}\
                    ]\
                }}'.format(SLACK_CHANNEL, results_fail, results_fail, results_pass, results_pass)
    headers = {
    'content-type': "application/json"
    }
    response = requests.request("POST", SLACK_TOKEN_URL, data=payload, headers=headers)
    print(response.text)

def write_to_file(content, file_path):
    with open(file_path, "w") as file:
        file.write(content)

def write_results_to_file(folder_path, results_pass, results_fail):
    overall_results = "PASSED"
    results = results_pass
    if results_fail != "":
        overall_results = "FAILED"
        results = "{}{}".format(results_fail, results)
    overall_results_file_path = "{}/{}".format(folder_path, OVERALL_RESULTS_FILE_NAME)
    results_file_path = "{}/{}".format(folder_path, RESULTS_FILE_NAME)
    write_to_file(overall_results, overall_results_file_path)
    write_to_file(results, results_file_path)

def handle_results(folder_path, results_pass, results_fail):
    post_results_to_slack(results_pass, results_fail)
    write_results_to_file(folder_path, results_pass, results_fail)

def parse_arguments(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input_dir',action='store', required=True,
                        help="Directory path of results files")
    options = parser.parse_args(args)
    return (parser, options)

if __name__ == '__main__':
    args = sys.argv[1:]
    parser, options = parse_arguments(args)
    result_folder_path = options.input_dir
    build_folder_path = os.path.dirname(result_folder_path)
    if os.path.isdir(result_folder_path):
        results_pass, results_fail = process_result_folder(result_folder_path)
        handle_results(build_folder_path, results_pass, results_fail)
    else:
        print("Error : Directory path of results files is not exist.")
