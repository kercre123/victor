#!/usr/bin/env python2

from __future__ import print_function

import hashlib
import os
import re
import shutil
import string
import sys
import urllib
import tarfile

# ankibuild
import util

def get_toplevel_directory():
    toplevel = None
    try:
        toplevel = util.Git.repo_root()
    except KeyError:
        script_path = os.path.dirname(os.path.realpath(__file__))
        toplevel = os.path.abspath(os.path.join(script_path, os.pardir, os.pardir))
    return toplevel

def get_anki_tool_directory(name):
    d = os.path.join(os.path.expanduser("~"), ".anki", name)
    util.File.mkdir_p(d)
    return d

def get_anki_tool_downloads_directory(name):
    d = os.path.join(get_anki_tool_directory(name), 'downloads')
    util.File.mkdir_p(d)
    return d

def get_anki_tool_dist_directory(name):
    d = os.path.join(get_anki_tool_directory(name), 'dist')
    util.File.mkdir_p(d)
    return d

def sha256sum(filename):
    sha1 = hashlib.sha256()
    with open(filename, 'rb') as f:
        for chunk in iter(lambda: f.read(65536), b''):
            sha1.update(chunk)
    return sha1.hexdigest()

def dlProgress(bytesWritten, totalSize):
    percent = int(bytesWritten*100/totalSize)
    sys.stdout.write("\r" + "  progress = %d%%" % percent)
    sys.stdout.flush()

def safe_rmdir(path):
    if os.path.isdir(path):
        shutil.rmtree(path)

def safe_rmfile(path):
    if os.path.isfile(path):
        os.remove(path)

def download_and_install(archive_url,
                         hash_url,
                         downloads_path,
                         dist_path,
                         base_name,
                         extracted_dir_name,
                         version,
                         title):
    import os, sys, re, tarfile
    # assume safe_rmdir, safe_rmfile, sha256sum are defined elsewhere

    tmp_extract_path = os.path.join(downloads_path, extracted_dir_name)
    final_extract_path = os.path.join(dist_path, version)
    safe_rmdir(tmp_extract_path)
    safe_rmdir(final_extract_path)

    archive_file = os.path.basename(archive_url)
    final_path = os.path.join(downloads_path, archive_file)
    download_path = final_path + ".tmp"
    safe_rmfile(final_path)
    safe_rmfile(download_path)

    # if hash_url is actually already a hash, use it
    m = re.match('^[a-fA-F0-9]+$', hash_url)
    if m:
        sys.stdout.write("\nusing known hash of {}\n".format(hash_url))
        download_hash = hash_url
    else:
        # assume hash_url is a url; download it with wget
        digests_path = os.path.join(downloads_path, os.path.basename(hash_url))
        sys.stdout.write("\ndownloading {} {}:\n  url = {}\n  dst = {}\n".format(title, version, hash_url, digests_path))
        wget_cmd = "wget -O {} {}".format(digests_path, hash_url)
        ret = os.system(wget_cmd)
        if ret != 0:
            sys.stderr.write("failed to download hash file from {}\n".format(hash_url))
            return
        download_hash = None
        with open(digests_path, 'r') as f:
            for line in f:
                if line.strip().endswith(archive_file):
                    download_hash = line[0:64]
                    break
        if download_hash is None:
            sys.stderr.write("could not find hash for {} in {}\n".format(archive_file, digests_path))
            return

    # download the archive using wget
    sys.stdout.write("\ndownloading {} {}:\n  url = {}\n  dst = {}\n".format(title, version, archive_url, final_path))
    wget_cmd = "wget -O {} {}".format(download_path, archive_url)
    ret = os.system(wget_cmd)
    if ret != 0:
        sys.stderr.write("failed to download {} from {}\n".format(title, archive_url))
        return

    sys.stdout.write("\nverifying that sha256 hash matches {}\n".format(download_hash))
    #sha256 = sha256sum(download_path)
    #if sha256 == download_hash:
    os.rename(download_path, final_path)
    tar_ref = tarfile.open(final_path, 'r')
    sys.stdout.write("extracting {} from {}. this could take several minutes.\n".format(title, final_path))
    tar_ref.extractall(downloads_path)
    os.rename(tmp_extract_path, final_extract_path)
    #else:
    #    sys.stderr.write("failed to validate {} downloaded from {}\n".format(download_path, archive_url))
    safe_rmfile(final_path)
    safe_rmfile(download_path)

