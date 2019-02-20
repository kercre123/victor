#!/usr/bin/env python3

import os
import sys
import argparse
import re

READ_MODE                    = "r"
WRITE_MODE                   = "w+"
NEW_LINE                     = "\n"
COMMENT_LIST                 = [ "\\\\", "\*", "*" ]
ALL_DAS_MSG_FILE_PATH        = "all_das_msg.cpp"
DAS_FILE                     = "DAS.h"
REGEX_FOR_DAS_MESSAGE        = r"DASMSG\(.*?DASMSG_SEND"
START_DAS_MESSAGE            = "/*! \defgroup dasmsg DAS Messages"
END_DAS_MESSAGE              = "#define DASMSG_SEND_DEBUG };"
BEGIN_HEADER_DAS_MESSAGES    = "// DAS feature start event"
END_HEADER_DAS_MESSAGES      = "// DAS message macros"
SUPPORTED_FILE_PATTERNS      = [ ".cpp" ]
GENERATED_FILES              = [ "das_new.cpp", "das_remove.cpp", ALL_DAS_MSG_FILE_PATH ]


def parse_arguments(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--path',
                        action='store',
                        default='../../',
                        help="Path of Victor project")
    options = parser.parse_args(args)
    return (parser, options)

def write_to_file(file_path, content):
    with open(file_path, WRITE_MODE) as file:
        file.write(content)

def read_file(file_path):
    content = ""
    with open(file_path, READ_MODE) as data_file:
        content = data_file.read()
    return content

def get_das_file():
    das_path = ""
    current_path = os.path.dirname(__file__)
    das_path = os.path.join(current_path, "util", "logging", DAS_FILE)
    return das_path

def get_das_structure():
    das = ""
    das_content = read_file(get_das_file())
    start_index = das_content.index(START_DAS_MESSAGE)
    end_index = das_content.index(END_DAS_MESSAGE)
    if start_index > 0 and end_index > start_index:
        das = das_content[start_index:end_index]
    return das

def get_das_headers():
    das = ""
    das_content = read_file(get_das_file())
    start_index = das_content.index(BEGIN_HEADER_DAS_MESSAGES)
    end_index = das_content.index(END_HEADER_DAS_MESSAGES)
    if start_index > 0 and end_index > start_index:
        das = das_content[start_index:end_index]
    return das

def build_das_msgs(all_das_msg):
    full_das_msgs = ""
    headers_das = get_das_headers()
    defined_das = get_das_structure()
    full_das_msgs = append_string(headers_das, defined_das)
    full_das_msgs = append_string(full_das_msgs, all_das_msg)
    return full_das_msgs

def append_string(src, new_value):
    return "{}{}{}".format(src, NEW_LINE, new_value)

def is_supported_file(file_path):
    is_supported = False
    for supported_pattern in SUPPORTED_FILE_PATTERNS:
        if file_path.endswith(supported_pattern):
            is_supported = True
            break
    return is_supported

def pick_das_msgs(file_path):
    content = read_file(file_path)
    regex = re.compile(REGEX_FOR_DAS_MESSAGE, re.DOTALL)
    res = regex.findall(content)
    return NEW_LINE.join(res)

def get_all_das_msgs(folder_path):
    all_das_msg = ""
    for path, subdirs, files in os.walk(folder_path):
        for name in files:
            file_path = os.path.join(path, name)
            if is_supported_file(file_path):
                das_msgs = pick_das_msgs(file_path)
                if das_msgs != "":
                    all_das_msg = append_string(all_das_msg, das_msgs)
    return all_das_msg

def clean_generated_files():
    for temp_file in GENERATED_FILES:
        write_to_file(temp_file, "")

def main():
    args = sys.argv[1:]
    parser, options = parse_arguments(args)
    root_folder = options.path

    clean_generated_files()
    all_das_msg = get_all_das_msgs(root_folder)
    full_das_msgs = build_das_msgs(all_das_msg)
    write_to_file(ALL_DAS_MSG_FILE_PATH, full_das_msgs)

if __name__ == "__main__":
    main()
