#!/usr/bin/env python3


#install bs4
#install lxml
import os
import sys
import argparse
from bs4 import BeautifulSoup
import requests

DASMSG                = "DASMSG"
DASMSG_SET            = "DASMSG_SET"
DASMSG_SEND           = "DASMSG_SEND();"
XML                   = "xml"
MEMBERDEF             = "memberdef"
PARAMETERITEM         = "parameteritem"
DAS_FILE              = "DAS.h"
DAS_NEW_FILE          = "das_new.cpp"
DAS_REMOVE_FILE       = "das_remove.cpp"
DAS_NEW_GROUP         = "das_msg_new"
DAS_REMOVE_GROUP      = "das_msg_remove"
NEW_DAS_MESSAGES      = "New DAS Messages"
REMOVE_DAS_MESSAGES   = "Remove DAS Messages"
START_DAS_MESSAGE     = "/*! \defgroup dasmsg DAS Messages"
END_DAS_MESSAGE       = "#define DASMSG_SEND_DEBUG };"
DAS_MESSAGE_GROUP     = "dasmsg"
DAS_MESSAGE_DESC      = "DAS Messages"
GROUP_DASMSG_FILE     = "group__dasmsg.xml"
DOWNLOAD_DOXYGEN_LINK = "https://build.ankicore.com/repository/download/Victor_Dev_DasDoxygenDocumentation"
REQUEST_SUCCESS_CODE  = 200


def parse_arguments(args):

    parser = argparse.ArgumentParser()
  
    parser.add_argument('-u', '--user_name',
                        action='store',
                        required=True,
                        help="user name on TeamCity")
    parser.add_argument('-p', '--password',
                        action='store',
                        required=True,
                        help="password on TeamCity")
    options = parser.parse_args(args)
    return (parser, options)

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

def build_group_das(group_name, group_description):
    group_das = ""
    das_template = get_das_structure()
    if len(das_template) > 0:
        group_das = das_template.replace(DAS_MESSAGE_GROUP, group_name)
        group_das = group_das.replace(DAS_MESSAGE_DESC, group_description)
    return group_das

def build_new_das_msgs(new_dasmsgs):
    new_dasmsgs_content = build_group_das(DAS_NEW_GROUP, NEW_DAS_MESSAGES)
    for das in new_dasmsgs:
        new_dasmsgs_content = "{}{}".format(new_dasmsgs_content, das)
    return new_dasmsgs_content

def build_remove_das_msgs(remove_dasmsgs):
    remove_dasmsgs_content = build_group_das(DAS_REMOVE_GROUP, REMOVE_DAS_MESSAGES)
    for das in remove_dasmsgs:
        remove_dasmsgs_content = "{}{}".format(remove_dasmsgs_content, das)
    return remove_dasmsgs_content

def write_to_file(content, file_path):
    with open(file_path, "w+") as data_file:
        data_file.write(content)

def read_file(file_path):
    content = ""
    with open(file_path, "r") as data_file:
        content = data_file.read()
    return content

def get_dasmsg(memberdef_xml):
    dasmsg = ""
    ez_ref = memberdef_xml.findAll("name")[0].get_text()
    event_name = memberdef_xml.briefdescription.para.get_text()
    documentation = memberdef_xml.detaileddescription.para.get_text()
    dasmsg = "{}({}, {}, {});".format(DASMSG, ez_ref, event_name, documentation)
    return dasmsg

def get_dasmsg_set(memberdef_xml):
    dasmsg_set = ""
    items = memberdef_xml.inbodydescription.findAll(PARAMETERITEM)
    for item in items:
        key = item.parameternamelist.parametername.get_text().strip()
        value = item.parameterdescription.para.get_text().strip()
        item_set = "{}({},{},{});".format(DASMSG_SET, key, 0, value)
        dasmsg_set = "{}\n{}".format(dasmsg_set, item_set)
    return dasmsg_set

def format_dasmsg(dasmsg, dasmsg_set, dasmsg_send):
    return "{}\n{}\n{}\n".format(dasmsg, dasmsg_set, dasmsg_send)

def parse_to_dasmsg(memberdef_xml):
    das_message = ""
    dasmsg = get_dasmsg(memberdef_xml)
    dasmsg_set = get_dasmsg_set(memberdef_xml)
    das_message = format_dasmsg(dasmsg, dasmsg_set, DASMSG_SEND)
    return das_message

def get_memberdefs(xml_content):
    memberdefs = []
    if xml_content != "":
        bs = BeautifulSoup(xml_content, XML)
        compounddef = bs.doxygen.compounddef
        if compounddef:
            memberdefs = compounddef.findAll(MEMBERDEF)
    return memberdefs

def parse_to_dasmsgs(memberdefs):
    all_das = []
    for memberdef in memberdefs:
        das_message = parse_to_dasmsg(memberdef)
        all_das.append(das_message)
    return all_das

def get_dasmsgs_from_xml(xml_content):
    all_das = []
    memberdefs = get_memberdefs(xml_content)
    if len(memberdefs) > 0:
        all_das = parse_to_dasmsgs(memberdefs)
    return all_das

def get_removed_dasmsgs(oldest_dasmsgs, current_dasmsgs):
    return [ das for das in oldest_dasmsgs if not das in current_dasmsgs ]

def get_new_dasmsgs(oldest_dasmsgs, current_dasmsgs):
    return [ das for das in current_dasmsgs if not das in oldest_dasmsgs ]

def get_dasmsgs_from_teamcity(user_name, password):
    all_das = []
    auth = (user_name, password)
    lastest_doxygen_url = "{}/.lastSuccessful/{}/{}".format(DOWNLOAD_DOXYGEN_LINK, XML, GROUP_DASMSG_FILE)
    r = requests.get(lastest_doxygen_url, auth=auth, allow_redirects=True)
    if r.status_code == REQUEST_SUCCESS_CODE:
        all_das = get_dasmsgs_from_xml(r.content)
    return all_das

def get_current_dasmsgs():
    xml_file_path = "./{}/{}".format(XML, GROUP_DASMSG_FILE)
    xml_content = read_file(xml_file_path)
    return get_dasmsgs_from_xml(xml_content)

def main():
    args = sys.argv[1:]
    parser, options = parse_arguments(args)
    user_name = options.user_name
    password = options.password
    
    current_dasmsgs = get_current_dasmsgs()
    oldest_dasmsgs = get_dasmsgs_from_teamcity(user_name, password)

    remove_dasmsgs = get_removed_dasmsgs(oldest_dasmsgs, current_dasmsgs)
    new_dasmsgs = get_new_dasmsgs(oldest_dasmsgs, current_dasmsgs)

    remove_dasmsgs_content = build_remove_das_msgs(remove_dasmsgs)
    new_dasmsgs_content = build_new_das_msgs(new_dasmsgs)

    write_to_file(remove_dasmsgs_content, DAS_REMOVE_FILE)
    write_to_file(new_dasmsgs_content, DAS_NEW_FILE)

if __name__ == "__main__":
    main()
