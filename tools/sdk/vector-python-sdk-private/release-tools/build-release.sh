#!/bin/bash

# This script will checkout the repo to a temporary directory
# and update version numbers ready for release.
#
# It creates two branches - branch_master_$SDK_VERSION to which it commits
# the changes that will ultimately want to be rebased to master
# and branch_local_$SDK_VERSION which holds the local changes necessary
# to build the actual release and documentation
#
# This script updates sdk/anki_vector/version.py
#
# It uses virtualenv to build the documentation and places the output in
# dist/docs which can then be opened in a browser locally to validate
#
# The path to the temporary directory containing the output of this script
# is printed as the final line of a successful run

set -e

if [ -z "$SDK_VERSION" ]; then
    echo "SDK_VERSION must be set"
    exit 1
fi

if [ -z "$SDK_VERSION_NEXT" ]; then
    echo "SDK_VERSION_NEXT must be set"
    exit 1
fi

if [ -z "$REPO_URL" ]; then
    echo "REPO_URL must be set"
    exit 1
fi

VDIR=$( mktemp -d )
CLONEDIR=$( mktemp -d )
SDKDIR=$CLONEDIR/vector-python-sdk

echo "Using virtualenv dir $VDIR"
echo "Using clone dir $VDIR"

virtualenv $VDIR
source $VDIR/bin/activate



cd $CLONEDIR
git clone $REPO_URL vector-python-sdk
cd vector-python-sdk

pip install sphinx sphinx-rtd-theme sphinx_autodoc_typehints twine PyOpenGL
pip install -r requirements.txt

# Build a branch with the changes we'd actually want to commit to master branch of public repo
git checkout -b build_master_$SDK_VERSION

python << END
SDK_VERSION_NEXT = '$SDK_VERSION_NEXT'

# update version.py to set to next version, like: "1.4.8.dev0
version_file = 'anki_vector/version.py'

buf = open(version_file).readlines()
with open(version_file, 'w') as f:
    for line in buf:
        if line.startswith('__version__'):
            line = '__version__ = "%s"\n' % SDK_VERSION_NEXT
        f.write(line)
END

git add -u anki_vector/version.py
git commit -m "Release $SDK_VERSION"



# Build a branch we'll actually use to build this release
# Note that we won't actually merge this branch or the commit created for it below.

git checkout master
git checkout -b build_local_$SDK_VERSION

python << END
SDK_VERSION = '$SDK_VERSION'

# update version.py
version_file = 'anki_vector/version.py'

buf = open(version_file).readlines()
with open(version_file, 'w') as f:
    for line in buf:
        if line.startswith('__version__'):
            line = '__version__ = "%s"\n' % SDK_VERSION
        f.write(line)
END


git add -u anki_vector/version.py
git commit -m "Temporary build of $SDK_VERSION"

make dist

# Build docs
cd docs
make clean
make html
cd ..
cp -a docs/build/html dist/

git checkout build_master_$SDK_VERSION 

deactivate
rm -rf $VDIR

echo SDK_DIR=$SDKDIR
