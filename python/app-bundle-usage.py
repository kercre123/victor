#!/usr/bin/env python2

# python app-bundle-usage.py --bundle build/android/Cozmo.apk

import argparse
import os
import subprocess
import sys

# order is important

android_groups = [
    { "prefix": "assets/bin/Data/Managed", "category": "Unity Code" },
    { "prefix": "assets/bin/Data", "category": "Unity Data" },
    { "prefix": "assets/AssetBundles", "category": "Unity Asset Bundle" },
    { "prefix": "assets/cozmo_resources/config/basestation/firmware", "category": "Firmware" },
    { "prefix": "assets/cozmo_resources/config/basestation/old_firmware", "category": "Firmware" },
    { "prefix": "assets/cozmo_resources/config/basestation/firmware_", "category": "Firmware" },
    { "prefix": "assets", "category": "Data" },

    { "prefix": "English", "category": "Audio English" },
    { "prefix": "German", "category": "Audio German" },
    { "postfix": ".wem", "category": "Audio Common" },
    { "postfix": ".bnk", "category": "Audio Common" },

    # C/C++ code for Unity
    { "prefix": "lib/armeabi-v7a/libmain.so", "category": "Unity Code" },
    { "prefix": "lib/armeabi-v7a/libmono.so", "category": "Unity Code" },
    { "prefix": "lib/armeabi-v7a/libunity.so", "category": "Unity Code" },

    # C/C++ libs
    { "prefix": "lib/armeabi-v7a", "category": "C/C++ Code" },

    { "prefix": "res", "category": "Android Data" },
    { "prefix": "classes.dex", "category": "Java Code"  },
    { "prefix": "AndroidManifest.xml", "category": "Android Data" },
    { "prefix": "META-INF", "category": "Android Data" },

    # Left over txt and xml are in the root of the audio zips
    { "postfix": ".txt", "category": "Audio Common" },
    { "postfix": ".xml", "category": "Audio Common" },
]

ios_groups = [
    { "prefix": "Payload/Cozmo.app/Data/Raw/cozmo_resources/config/basestation/firmware", "category": "Firmware" },
    { "prefix": "Payload/Cozmo.app/Data/Raw/cozmo_resources/config/basestation/old_firmware", "category": "Firmware" },
    { "prefix": "Payload/Cozmo.app/Data/Raw/cozmo_resources/config/basestation/firmware_", "category": "Firmware" },
    { "prefix": "Payload/Cozmo.app/Data/Raw/AssetBundles", "category": "Unity Asset Bundle" },
    { "prefix": "Payload/Cozmo.app/Data/Raw", "category": "Data" },
    { "prefix": "Payload/Cozmo.app/Data", "category": "Unity Data" },

    # # contents of the audio zips
    { "prefix": "English", "category": "Audio English" },
    { "prefix": "German", "category": "Audio German" },
    { "postfix": ".wem", "category": "Audio Common" },
    { "postfix": ".bnk", "category": "Audio Common" },
    { "postfix": ".txt", "category": "Audio Common" },

    # # C/C++ libs including Unity runtime
    { "prefix": "Payload/Cozmo.app/cozmo", "category": "C/C++ Code" },
    { "prefix": "", "category": "iOS Data" },
]

zips = []

def update(groups, stats, lines):

    # Skip
    #    Archive:  /Users/richard/src/overdrive-one/build/gradle/android/outputs/apk/OverDrive.apk
    #     Length   Method    Size  Ratio   Date   Time   CRC-32    Name
    #    --------  ------  ------- -----   ----   ----   ------    ----

    max_lines = len(lines)
    lines = lines[3:max_lines-3]

    for line in lines:

        # Split
        #  Length   Method    Size  Ratio   Date   Time   CRC-32    Name

        columns = line.split()
        length = int(columns[0])
        size = int(columns[2])
        name = columns[7]

        if length == 0:
            # ignore directories in output
            continue

        if ".obb" in name or ".zip" in name:
            # ignore obb/zip file sizes, process separately
            zips.append(name)
            continue

        for group in groups:
            if "prefix" in group:
                match = name.startswith(group["prefix"])
            elif "postfix" in group:
                match = name.endswith(group["postfix"])
            else:
                match = False

            if match:
                category = group["category"]
                for i in (category, "total"):
                    if i not in stats:
                        stats[i] = {}
                        stats[i]["length"] = 0
                        stats[i]["size"] = 0
                        stats[i]["count"] = 0

                stats[category]["length"] += length
                stats[category]["size"] += size
                stats[category]["count"] += 1

                stats["total"]["length"] += length
                stats["total"]["size"] += size
                stats["total"]["count"] += 1

                break

def prettybytes(value, total):
    if value >= 1024*1024:
        return (value/(1024*1024), "MB", value/float(total))
    if value >= 1024:
        return (value/(1024), "KB", value/float(total))
    return (value, "B ", value/float(total))

def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--bundle', dest='bundle', required = True, default="build/gradle/android/outputs/apk/OverDrive.apk")
    parser.add_argument('--json', action='store_true', dest='asjson', default=False)

    options = parser.parse_args()
    file = options.bundle

    if os.path.isfile(file):
        unused, ext = os.path.splitext(file)

        if ext == ".apk":
            groups = android_groups
        elif ext == ".ipa":
            groups = ios_groups
        else:
            return

        stats = {}
        output = subprocess.Popen("unzip -vl %s" % (file), shell=True, stdout=subprocess.PIPE).stdout.read()
        lines = output.split('\n')
        update(groups, stats, lines)

        for zip in zips:
            output = subprocess.Popen("unzip -o %s %s" % (file, zip), shell=True, stdout=subprocess.PIPE).stdout.read()
            output = subprocess.Popen("unzip -vl %s" % (zip), shell=True, stdout=subprocess.PIPE).stdout.read()
            os.remove(zip)
            lines = output.split('\n')
            update(groups, stats, lines)

        if not options.asjson:
            # Human readable report

            summary_category = []
            for group in groups:
                if group["category"] not in summary_category:
                    summary_category.append(group["category"])

            summary_category.sort()

            print file
            for category in summary_category:
                if category in stats:
                    length = stats[category]["length"]
                    size = stats[category]["size"]
                    count = stats[category]["count"]

                    lengthtuple = prettybytes(length, stats["total"]["length"])
                    sizetuple = prettybytes(size, stats["total"]["size"])
                    print "%20s: uncompressed %3d %s (%0.2f) compressed %3d %s (%0.2f) count %d" % (category, lengthtuple[0], lengthtuple[1], lengthtuple[2], sizetuple[0], sizetuple[1], sizetuple[2],count)

            print
            length = stats["total"]["length"]
            size = stats["total"]["size"]
            count = stats["total"]["count"]
            lengthtuple = prettybytes(length, length)
            sizetuple = prettybytes(size, size)
            print "%20s: uncompressed %3d %s (%0.2f) compressed %3d %s (%0.2f) count %d" % ("Total", lengthtuple[0], lengthtuple[1], lengthtuple[2], sizetuple[0], sizetuple[1], sizetuple[2],count)

        else:
            # Json report for build labs
            stats["file"] = file
            print stats

if __name__ == '__main__':
    ret = main()
    sys.exit(ret)
