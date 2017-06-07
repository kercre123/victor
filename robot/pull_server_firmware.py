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
    djson = json.load(open(deps_file, 'r'))
    return djson["teamcity"]

def setup_auth(root_url, user, password):
    # create a password manager
    password_mgr = request.HTTPPasswordMgrWithDefaultRealm()
    # Add the username and password.
    password_mgr.add_password(None, root_url, user, password)
    handler = request.HTTPBasicAuthHandler(password_mgr)
    # create "opener" (OpenerDirector instance)
    return request.build_opener(handler)

def pull_firmware(deps_file, version_override = None):
    teamcity  = get_tc_dep(deps_file)
    root_url = teamcity['root_url']
    urlOpener = setup_auth(root_url, teamcity['default_usr'], teamcity['pwd'])
    build = teamcity["builds"][FIRMWARE_BUILD_KEY]
    version = build.get("version", "undefined")
    if version_override :
       version = version_override
       print("override: version {}".format(ver_override))
    build_type_id = build.get("build_type_id", "undefined")
    package_name = build.get("package_name", "undefined")
    ext = build.get("extension", "undefined")
    print("Downloading firmware version {}".format(version))
    combined_url = "{0}/repository/download/{1}/{2}/{3}_{4}.{5}".format(root_url, build_type_id, version, package_name, version, ext)
    resp = urlOpener.open(combined_url)
    destFilename = "{0}.{1}".format(package_name, ext)
    dest = open(destFilename, "wb")
    dest.write(resp.read())
    print("Unpacking", destFilename)
    shutil.unpack_archive(destFilename)
    os.remove(destFilename)

if __name__ == '__main__':
    if 'old' in sys.argv:
        FIRMWARE_BUILD_KEY = 'old_firmware'
    try:
        vpos = sys.argv.index("-v")+1
        ver_override = int(sys.argv[vpos])
        print("override: version {}".format(ver_override))
    except:
        ver_override = None
        
    pull_firmware(DEPS_FILE, ver_override)
