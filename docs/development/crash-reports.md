# Victor Crash Reports

Victor has crash reporting functionality that uses Google Breakpad.  The Google Breakpad client is built for VICOS and is integrated into vic-engine, vic-anim, vic-robot, vic-switchboard and vic-webserver.  We have NOT yet implemented crash reporting for vic-cloud as this requires Go language support.  We also don't use this for kernel crashes.

## Where to find crash report files

When a crash occurs you'll see something like this in the log:

```
05-16 17:31:26.051  1777  1810 I vic-engine: [@GoogleBreakpad] GoogleBreakpad.DumpCallback: (tc88469) : Dump path: '/data/data/com.anki.victor/cache/crashDumps/engine-1970-01-01T00-00-14-595.dmp', fd = 3, context = (nil), succeeded = true
05-16 17:31:26.084  1776  1776 I vic-engine: vic-engine terminated by signal 11
```

The crash report file is found on the robot at `/data/data/com.anki.victor/cache/crashDumps/` and has a name like `engine-1970-01-01T00-00-14-595.dmp` where the prefix is the name of the process that crashed, and the date/time stamp is when the process *started*.  The file is in standard "minidump" format.

When Victor is running you may see these files with a size of zero.  That's because we open the file when the process starts.  When Victor shuts down normally those files should disappear.

## Automatic uploading/deletion

Every 60 seconds a process called vic-crashuploader runs.  It looks for files ending in ".dmp" in the crash folder that have a non-zero size and are not held by any process, and attempts to upload them to Backtrace IO, our third-party crash reporting service.  If the upload succeeds the file is renamed to append ".uploaded", and we keep the newest 50 ".uploaded" files on the robot so we can get a chance to examine them, without filling up robot storage.

## Backtrace IO and symbolication of call stacks (WIP)

The URL for our Backtrace IO account is https://anki.sp.backtrace.io/dashboard/anki and the project name is 'victor'.  See Jane Fraser for an account if you need one.

The build team is currently working on adding a build step that will automatically generate the symbols for a build in the proper format, and upload them to Backtrace IO.  Once this work is completed we'll be able to see symbolicated call stacks on Backtrace IO for any crash that occurs on a build for which the symbols have been generated.

In the meantime, you must manually do this...

## Manually create symbols and a call stack

The idea here is to copy your 'full' binaries to a Linux VM, then run a script/tool to generate a symbols package, then run another tool to generate a call stack.

The tools need to run on a Linux VM. In the victor repo they can be found (along with these rough scripts) at `tools/crash-tools/linux`.  At some point we will clean these up into a simpler solution so this is more automated.


First, you need the `*.full` files from your build (that had the crash).  The processing of those files to create the symbols must be run on a Linux VM (NOT OSX), so first copy them to a VM, using a script like this:  [`copyFullBinariesToVM.sh`](/tools/crash-tools/linux/copyFullBinariesToVM.sh)

```
#!/bin/bash

set -e

REMOTE_HOST='pterry@10.10.7.148'
DEST_FOLDER='/home/pterry/test'
SRC_FOLDER='../github/Victor/_build/vicos/Release'

echo "Copying *.full files from here to my VM"
ssh $REMOTE_HOST "rm -rf $DEST_FOLDER/lib"
ssh $REMOTE_HOST "rm -rf $DEST_FOLDER/bin"
ssh $REMOTE_HOST "mkdir $DEST_FOLDER/lib"
ssh $REMOTE_HOST "mkdir $DEST_FOLDER/bin"
scp $SRC_FOLDER/lib/*.full $REMOTE_HOST:$DEST_FOLDER/lib/
scp $SRC_FOLDER/bin/*.full $REMOTE_HOST:$DEST_FOLDER/bin/
echo "DONE"
```

Next, on the VM, run the `dump_syms` tool (that has been built for Linux) with a script like this:  [`makeSyms.sh`](/tools/crash-tools/linux/makeSyms.sh)

```
#!/bin/bash

set -e

echo "Renaming *.full -> * in bin and lib folders"
pushd bin
rename 's/\.full$//' *.full
popd # bin
pushd lib
rename 's/\.so\.full$/\.so/' *.so.full
popd # bin

rm -rf symbols

echo "Making symbols for * in bin folder"
pushd bin
for i in `ls *`; do
    echo Processing ${i}...
    ./../dump_syms $i . > $i.sym || ./../dump_syms -v $i > $i.sym
    VER=`head -1 $i.sym | awk '{print $4;}'`
    mkdir -p ../symbols/$i/$VER
    mv $i.sym ../symbols/$i/$VER/
done
popd # bin

echo "Making symbols for *.so in lib folder"
pushd lib
for i in `ls *.so`; do
    echo Processing ${i}...
    ./../dump_syms $i . > $i.sym || ./../dump_syms -v $i > $i.sym
    VER=`head -1 $i.sym | awk '{print $4;}'`
    mkdir -p ../symbols/$i/$VER
    mv $i.sym ../symbols/$i/$VER/
done
popd # lib
```

Now copy the symbols off the VM and package them with something like this:  [`copySymsFromVM.sh`](/tools/crash-tools/linux/copySymsFromVM.sh)

```#!/bin/bash

set -e

REMOTE_HOST='pterry@10.10.7.148'
REMOTE_FOLDER='/home/pterry/test'

echo "Copying symbols from VM"
rm -rf symbols
scp -rp $REMOTE_HOST:$REMOTE_FOLDER/symbols/ .

echo "Creating compressed tar file"
rm -f symbols.tar.gz
tar -czf symbols.tar.gz symbols/

echo "DONE"
```

Finally, run the `minidump_stackwalk` tool, giving it the name of the symbols package, and the name of the crash dump file, and it outputs a call stack.  This must be run on Linux OS, not OSX or VICOS.

