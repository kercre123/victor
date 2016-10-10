# Cozmo Python SDK CLAD Package

This directory contains build steps for generating the Python `cozmoclad`
package.

The package only contains the Python CLAD libraries and is used by the
main `cozmo` Python SDK package.

The SDK itself lives in its own repo at
https://github.com/anki/cozmo-python-sdk

## Working with a build from TeamCity

Download the `cozmoclad*.whl` artifact and install it using
`pip3 install -I <filename>` (The -I update will cause pip to ignore any
currently installed version).

Clone the Python SDK repo and run `pip install -e .` from that directory
to make Python use the source files within that repo as the `cozmo` package.


## Working with a local build of the cozmo-one repo

* Ensure the CLAD files are built.
* Run `make copy-clad` from this directory to copy the Python clad files into the package.
* `pip3 install -e .` in this directory to make Python use the source files within
this directory as the `cozmoclad` package
* Install the `cozmo-python-sdk` package from that repo.
