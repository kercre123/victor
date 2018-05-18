#!/bin/bash
cd "$(dirname ${BASH_SOURCE[0]})/release"

#get recursive list of git tracked files in tools/release
echo collecting file list...
current_branch=$(git rev-parse --abbrev-ref HEAD)
filez=$(git ls-tree -r $current_branch --name-only) # | grep "release/")
uncomittedfilez=$(git ls-files -m) # | grep "release/")

#build the manifest
manifest=manifest
echo $(printf timestamp:%s "$(date +'%m/%d/%Y %H:%M:%S')") > $manifest
echo $(printf branch:%s $(git rev-parse --abbrev-ref HEAD)) >> $manifest
echo $(printf sha-1:%s $(git rev-parse HEAD)) >> $manifest
if [ "$uncomittedfilez" == "" ]; then isclean=1; else isclean=0; fi
echo $(printf working-tree-clean:%d $isclean) >> $manifest
echo $(printf working-tree-changes:%s "$uncomittedfilez") >> $manifest

#build the zip
echo zipping files...
zipfile="$(date +'%Y%m%d.%H%M')_tools.zip" && rm -f $zipfile && sleep 1
zip -9T $zipfile $filez $manifest
#cp $manifest "../$(date +'%Y%m%d.%H%M')_manifest.mf" #DEBUG
rm -f $manifest
mv $zipfile ../$zipfile #move zip back to script location
