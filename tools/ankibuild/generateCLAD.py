#!/usr/bin/env python

import os
import subprocess
import time
import md5
import re

# ankibuild
import util

def generateMessageBuffers(basestation_dir, unity_dir, clad_emitter_dir=None, verbose=False):
    message_buffers_src=os.path.join(basestation_dir, 'source', 'anki', 'clad')
    message_buffers_dst=os.path.join(unity_dir, 'Assets', 'Libs', 'Anki', 'Messages')
    message_buffers_dst_rel=os.path.relpath(message_buffers_dst, message_buffers_src)
    print "[CLAD] Generating C# Sources..."

    if not clad_emitter_dir:
        clad_emitter_dir = os.path.join(util.Module.get_path('clad'), 'emitters')

    stdout = subprocess.PIPE
    if verbose:
        print "src -> {0}".format(basestation_dir)
        print "dst -> {0}".format(unity_dir)
        stdout = None

    make_vars = {
        "CLAD_EMITTER_DIR": clad_emitter_dir,
        "OUTPUT_DIR": message_buffers_dst_rel
    }

    make_params = ['make',
                   '-j', '6',
                   '-C', message_buffers_src]

    for k,v in make_vars.iteritems():
        var_def = "%s=%s" % (k,v)
        make_params.append(var_def)

    make_params.append('csharp')

    result = subprocess.call(make_params, stdout=stdout)
    if (result == 0):
        generateMetaFiles(message_buffers_dst, verbose)
    return result

def generateMetaFiles(path, verbose=False):
    meta_file_template = """fileFormatVersion: 2
guid: %(path_md5)s
folderAsset: %(is_folder_asset)s
timeCreated: %(creation_time_secs)d
licenseType: Pro
DefaultImporter:
  userData:
  assetBundleName:
  assetBundleVariant:
"""
    print('[CLAD] Generating .meta files...')
    for dir_name, subdir_list, file_list in os.walk(path):
        dir_rel_path=os.path.relpath(dir_name, os.path.join(path, '..'))
        if(verbose):
            print(dir_rel_path)
        with open(os.path.join(dir_name + '.meta'), 'w+') as dir_meta_file:
            dir_meta_file.write(meta_file_template % { 'path_md5': md5.new(dir_rel_path).hexdigest(),
                                                       'is_folder_asset': 'yes',
                                                       'creation_time_secs': int(time.time())
                                                   })
        for file_name in [i for i in file_list if not re.match(r'.*\.meta$', i)]:
            file_path = os.path.join(dir_name, file_name)
            file_rel_path = os.path.join(dir_rel_path, file_name)
            if(verbose):
                print(file_rel_path)
            with open(file_path + '.meta', 'w+') as meta_file:
                meta_file.write(meta_file_template % { 'path_md5': md5.new(file_rel_path).hexdigest(),
                                                       'is_folder_asset': 'no',
                                                       'creation_time_secs': int(time.time())
                                                   })
        

if __name__ == '__main__':
    import sys
    #ankibuild
    import util
    from unity import UnityBuildConfig

    repo_root = util.Git.repo_root()
    basestation_dir = util.Module.get_path('basestation')
    unity_dir = UnityBuildConfig.find_unity_project(repo_root)
    result = generateMessageBuffers(basestation_dir, unity_dir, verbose=False)
                                    
    if not result:
        sys.exit(1)
    
