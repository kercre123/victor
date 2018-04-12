#!/bin/bash

hash git 2>/dev/null || { echo >&2 "'git' required but not installed. Aborting."; exit 1; }
hash node 2>/dev/null || { echo >&2 "'node' required but not installed. Aborting."; exit 1; }
hash npm 2>/dev/null || { echo >&2 "'npm' required but not installed. Aborting."; exit 1; }
hash python3 2>/dev/null || { echo >&2 "'python3' required but not installed. Aborting."; exit 1; }

if [ ! -d /tmp/pynacl ]; then
    # Note: this can change to a pip install when 1.3.0 is released
    echo "Pulling $(tput setaf 13)pyca/pyncal$(tput sgr0)"
    git clone https://github.com/pyca/pynacl.git /tmp/pynacl
fi

echo "Installing $(tput setaf 13)pyca/pyncal$(tput sgr0)... This may take a while."
python3 -m pip install /tmp/pynacl
python3 -m pip install termcolor

pushd "$(git root)/victor-clad"
python3 configure.py --python
popd
rm -rf victorclad
cp -R "$(git root)/victor-clad/generated/cladPython/clad" victorclad

rm -rf msgbuffers
cp -R "$(git root)/victor-clad/tools/message-buffers/support/python/msgbuffers" msgbuffers

echo "Installing $(tput setaf 13)npm packages$(tput sgr0)..."
pushd node-ble
npm install
popd