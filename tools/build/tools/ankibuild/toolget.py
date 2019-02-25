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
import boto3

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

def download_and_install_from_s3(s3_params,
                                 sha256,
                                 downloads_path,
                                 dist_path,
                                 base_name,
                                 extracted_dir_name,
                                 version,
                                 title):
    tmp_extract_path = os.path.join(downloads_path, extracted_dir_name)
    final_extract_path = os.path.join(dist_path, version)
    safe_rmdir(tmp_extract_path)
    safe_rmdir(final_extract_path)

    archive_file = sha256

    final_path = os.path.join(downloads_path, archive_file)
    download_path = final_path + ".tmp"
    safe_rmfile(final_path)
    safe_rmfile(download_path)

    session = boto3.Session(profile_name = s3_params.get('profile_name', None))
    client = session.client('s3')

    response = client.get_object(Bucket=s3_params['bucket'],
                                 Key=s3_params['key'])
    handle = response['Body']
    download_file = open(download_path, 'w')
    block_size = 1024 * 1024
    sys.stdout.write("\nDownloading {0} {1} from s3:\nbucket = {2}\nkey    = {3}\ndst    = {4}\n"
                     .format(title, version, s3_params['bucket'], s3_params['key'], final_path))
    for chunk in iter(lambda: handle.read(block_size), b''):
        download_file.write(chunk)

    download_file.close()
    sys.stdout.write("\n")
    sys.stdout.write("Verifying that SHA256 hash matches {0}\n".format(sha256))
    download_sha256 = sha256sum(download_path)
    if download_sha256 == sha256:
        os.rename(download_path, final_path)
        tar_ref = tarfile.open(final_path, 'r')
        sys.stdout.write("Extracting {0} from {1}.  This could take several minutes.\n"
                         .format(title, final_path))
        print("extractall %s" % downloads_path)
        tar_ref.extractall(downloads_path)
        print("rename {} -> {}".format(tmp_extract_path, final_extract_path))
        os.rename(tmp_extract_path, final_extract_path)
    else:
        sys.stderr.write("Failed to validate {0} downloaded from s3 (bucket = {1} , key = {2})\n"
                         .format(download_path, s3_params['bucket'], s3_params['key']))

    safe_rmfile(final_path)
    safe_rmfile(download_path)

def download_and_install(archive_url,
                         hash_url,
                         downloads_path,
                         dist_path,
                         base_name,
                         extracted_dir_name,
                         version,
                         title):
    tmp_extract_path = os.path.join(downloads_path, extracted_dir_name)
    final_extract_path = os.path.join(dist_path, version)
    safe_rmdir(tmp_extract_path)
    safe_rmdir(final_extract_path)

    archive_file = os.path.basename(archive_url)

    final_path = os.path.join(downloads_path, archive_file)
    download_path = final_path + ".tmp"
    safe_rmfile(final_path)
    safe_rmfile(download_path)


    # see if hash_url is actually already a hash
    m = re.match('^[a-fA-F0-9]+$', hash_url)
    if m:
        sys.stdout.write("\nUsing known hash of {}\n".format(hash_url))
        download_hash = hash_url
    else:
        # not a hash, assume url
        handle = urllib.urlopen(hash_url)
        code = handle.getcode()

        archive_file = os.path.basename(archive_url)
        download_hash = None

        if code >= 200 and code < 300:
            digests_path = os.path.join(downloads_path, os.path.basename(hash_url))
            download_file = open(digests_path, 'w')
            block_size = 1024 * 1024
            sys.stdout.write("\nDownloading {0} {1}:\n  url = {2}\n  dst = {3}\n"
                             .format(title, version, hash_url, digests_path))
            for chunk in iter(lambda: handle.read(block_size), b''):
                download_file.write(chunk)

            download_file.close()
            with open(digests_path, 'r') as f:
                for line in f:
                    if line.strip().endswith(archive_file):
                        download_hash = line[0:64]


    handle = urllib.urlopen(archive_url)
    code = handle.getcode()
    if code >= 200 and code < 300:
        download_file = open(download_path, 'w')
        block_size = 1024 * 1024
        sys.stdout.write("\nDownloading {0} {1}:\n  url = {2}\n  dst = {3}\n"
                         .format(title, version, archive_url, final_path))
        for chunk in iter(lambda: handle.read(block_size), b''):
            download_file.write(chunk)

        download_file.close()
        sys.stdout.write("\n")
        sys.stdout.write("Verifying that SHA256 hash matches {0}\n".format(download_hash))
        sha256 = sha256sum(download_path)
        if sha256 == download_hash:
            os.rename(download_path, final_path)
            tar_ref = tarfile.open(final_path, 'r')
            sys.stdout.write("Extracting {0} from {1}.  This could take several minutes.\n"
                             .format(title, final_path))
            print("extractall %s" % downloads_path)
            tar_ref.extractall(downloads_path)
            print("rename {} -> {}".format(tmp_extract_path, final_extract_path))
            os.rename(tmp_extract_path, final_extract_path)
        else:
            sys.stderr.write("Failed to validate {0} downloaded from {1}\n"
                             .format(download_path, archive_url))
    else:
        sys.stderr.write("Failed to download {0} {1} from {2} : {3}\n"
                         .format(title, version, archive_url, code))
    safe_rmfile(final_path)
    safe_rmfile(download_path)
