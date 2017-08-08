#!/usr/bin/env python3

"""
replace_filename_spaces.py

Replaces spaces with underscores in both VC test file names and the json data
that references them.
"""

import json
import os


def update_json_filenames(path):
    """Updates specified VC test sample JSON file and converts filename spaces to underscores"""
    with open(path, 'r') as fp:
        data = json.loads(fp.read().replace('\n', ''))
    made_change = False
    for item in data:
        old_name = item["sampleFilename"]
        if " " in old_name:
            made_change = True
            new_name = old_name.replace(" ", "_")
            item["sampleFilename"] = new_name

    if made_change:
        with open(path, 'w') as fp:
            json.dump(data, fp, indent=2, sort_keys=True)


def replace_spaces(path):
    """Uses specified path and converts all spaces in filenames to underscores and updates VC JSON data accordingly"""
    for root, dirs, files in os.walk(path):
        for f in files:
            if f == "samplePhraseData.json":
                update_json_filenames(os.path.join(root, f))
            if " " in f:
                new_name = f.replace(" ", "_")
                os.rename(os.path.join(root,f), os.path.join(root,new_name))

if __name__ == "__main__":
    replace_spaces('.')
