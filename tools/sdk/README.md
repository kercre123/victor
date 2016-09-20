# Cozmo SDK

See online documentation at http://cozmosdk.anki.com/docs/


# Local development of the SDK

## Install CLAD files

Download the build of the SDK that matches the version of Cozmo you're running
on your device from TeamCity and copy the `cozmo/_internal/clad` and 
`cozmo/internal/msgbuffers` directory contents into the corresponding
directories here.

Alternatively if you're running a build of the app from this repo, run
`make copy-clad` to copy the CLAD files into the Cozmo source directory.

## Install the cozmo package

Install all of the dependencies for the library and make the cozmo
directory available on the python path:

`pip install -e '.[camera,test]'`

This causes pip to install numpy and Pillow require for image processing,
along wtih tox and pytest required for running unit tests.

## Run the tests

Run `tox` to run the tests - This will setup a virtualenv environment,
build and install the package and then run the tests, ensuring a clean
end-user like environment for testing.

Alternatively run `pytest` by itself to test in your current environment.


# Build the docs

````bash
pip install sphinx
cd docs && make html
````
