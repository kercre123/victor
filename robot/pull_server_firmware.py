#!/usr/bin/env python3
"""
Script to pull firmware for current repository revision build on the server.
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, json, shutil
from urllib import request, error

DEPS_FILE = os.path.join("..", "DEPS")
FIRMWARE_BUILD_KEY = "firmware"

def get_tc_dep(deps_file):
    if not os.path.isfile(deps_file):
        sys.exit("ERROR: Deps file \"{}\" doesn't exist".format(deps_file))
    djson = json.load(open(deps_file, 'r')
    return djson["teamcity"]

def setup_auth(root_url, user, password):
    # create a password manager
    password_mgr = urllib.request.HTTPPasswordMgrWithDefaultRealm()
    # Add the username and password.
    password_mgr.add_password(None, root_url, user, password)
    handler = urllib.request.HTTPBasicAuthHandler(password_mgr)
    # create "opener" (OpenerDirector instance)
    return urllib.request.build_opener(handler)

def pull_firmware(deps_file):
    teamcity  = get_tc_dep(deps_file)
    urlOpener = setup_auth(teamcity['root_url'], teamcity['default_usr'], teamcity['pwd'])
    build = teamcity["builds"][FIRMWARE_BUILD_KEY]
    build_id = build.get("build_id", "undefined")
    version = build.get("version", "undefined")
    build_type_id = build.get("build_type_id", "undefined")
    package_name = build.get("package_name", "undefined")
    ext = build.get("extension", "undefined")
    combined_url = "{0}/repository/download/{1}/{2}:id/{3}_{4}.{5}".format(root_url, build_type_id, build_id, package_name, version, ext)
    resp = urlOpener.open(combined_url)
    destFilename = "{0}.{1}".format(package_name, ext)
    dest = open(destFilename, "wb")
    dest.write(resp.read())
    shutil.unpack_archive(destFilename)

def pull():
    pass
    
    
    

if __name__ == '__main__':
    pull()
