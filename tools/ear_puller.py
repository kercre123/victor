#!/usr/bin/env python3
"""
A simple script to grab mic capture files from a Victor robot.

The ear_puller python script uses the web interface on the anim process of the robot to pull down mic capture files and optionally clear them.
"""

import argparse
import bs4
import datetime
import os
import requests
import socket
import subprocess
import sys
import urllib.request

def get_file_links(location):
    """Parses the page at location and returns a list of links on the page, skipping the first (assumed to be parent dir)."""
    try:
        response = requests.get(location)
    except OSError:
        print('Could not connect to {}'.format(location))
        exit()

    soup = bs4.BeautifulSoup(response.text, "html.parser")
    # the first is always the parent directory, so get rid of it
    return [a.attrs.get('href') for a in soup.select("table:nth-of-type(1) > tr > td > a")][1:]

def get_filepaths(filedir, host):
    """Uses host and directory to recursively find file links."""
    resultlist = []
    # Parse the page and collect the links
    links = get_file_links(host + filedir)
    for link in links:
        if link.endswith('/'):
            resultlist += get_filepaths(link, host)
        else:
            resultlist.append(link)
    return resultlist

def ear_puller(host):
    """Uses the given host address to find and download all micdata files on the robot."""
    basedir = '/cache/micdata/'
    filepaths = get_filepaths(basedir, host)
    filepathcount = len(filepaths)
    if filepathcount > 0:
        # figure out where we're going to save files, using the current month, date, and time to name the folder
        savedir = os.path.join('~','Downloads','micdata{0:%m%d-%k%M%S}'.format(datetime.datetime.now()))
        savedir = os.path.expanduser(savedir)

        count = 1
        print('{} to pull from {}'.format(filepathcount, host))
        for fp in filepaths:
            savepath = fp.replace(basedir, '')
            savepath = os.path.join(savedir, savepath)
            savepath = os.path.normpath(savepath)
            os.makedirs(os.path.dirname(savepath), exist_ok=True)
            srcpath = host + fp
            urllib.request.urlretrieve(srcpath, savepath)
            print('{} {}'.format(count, savepath))
            count += 1
        # open the folder we just saved things into
        subprocess.run(['open', savedir])
    else:
        print('no files found')

def get_host(ip):
    """Uses the given ip address to construct the base url to connect to."""
    port = '8889'
    host = 'http://' + ip + ':' + port

    # validate we can talk to the host
    try:
        response = requests.get(host, timeout=4)
    except OSError:
        print('Could not connect to {}'.format(ip))
        exit()
    return host

if __name__=="__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("ip", help="ip address to pull data from")
    parser.add_argument("-c", "--clear", help="clear data after pulling", action="store_true")
    args = parser.parse_args()
    ipaddr = args.ip

    # validate the ip address argument
    try:
        socket.inet_aton(ipaddr)
        # legal
    except socket.error:
        print ('Not a valid ip {}'.format(ipaddr))
        exit()
        # Not legal
    host = get_host(ipaddr)
    # pull the data
    ear_puller(host)

    # if requested, clear the data
    if args.clear:
        cleararg = '/consolefunccall?func=ClearMicData'
        try:
            requests.get(host + cleararg)
        except OSError:
            print('Could not connect to {}'.format(location))
            exit()
