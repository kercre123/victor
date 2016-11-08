#!/usr/bin/env python3
import http.client
from base64 import b64encode
import subprocess
import xml.etree.ElementTree as ElementTree
import json
import argparse
import getpass

def execute(*kargs):
    p = subprocess.Popen(kargs, stdout=subprocess.PIPE)
    assert p.wait() == 0
    return p.communicate()[0].decode()

def git_sha():
    return execute('git', 'rev-parse', 'HEAD').strip()

def getBuild(user, password):
    connection = http.client.HTTPSConnection('build.ankicore.com')

    headers = { 
        'Authorization' : 'Basic %s' % b64encode(b"%s:%s" % (user, password)).decode('ascii')
    }

    connection.request("GET", "/httpAuth/app/rest/builds?locator=buildType:(id:CozmoOne_Experiments_CollectFirmware),branch:(default:any),revision:%s" % git_sha(), headers=headers)
    res = connection.getresponse()
    data = res.read()

    tree = ElementTree.fromstring(data)

    for child in tree:
        status, id, number = child.attrib['status'], child.attrib['id'], child.attrib['number']
        if (status != "SUCCESS"):
            raise Exception("Build status is %s" % status)

        return (id, number)

    raise Exception("Build not found")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Update firmware DEPS.')
    parser.add_argument('user', type=str)
    parser.add_argument('-p','--password', type=str)
    parser.add_argument('-d', '--DEPS', type=str, default="DEPS")

    args = parser.parse_args()
    if args.password is None:
        args.password = getpass.getpass()

    user, password = bytes(args.user,'utf-8'), bytes(args.password,'utf-8')

    build_id, version = getBuild(user, password)

    with open(args.DEPS, "r") as fo:
        config = json.load(fo)

    config['teamcity']['builds']['firmware']['version'] = version

    with open(args.DEPS, "w") as fo:
        json.dump(config, fo, sort_keys=True, indent=4)
        fo.write("\n")
