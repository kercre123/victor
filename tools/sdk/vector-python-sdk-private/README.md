# Vector Python SDK Private

This is the private repository for the Vector Python SDK.
The code under the `sdk` directory is planned for release to the public.

```bash
.../vector-python-sdk-private
├── release-tools     # Instructions and tools for releasing the SDK.
├── sdk               # Source to be copied to the public SDK repo during release.
│   ├── anki_vector   # Python module that makes up the SDK.
│   │   └── configure # Executable module to set up the SDK with your Vector (production account).
│   │   └── messaging # Contains all of the protobuf code. update.py creates the generated pb2 files,
│   │                 # and copies and modifies the proto files here. Also contains __init__.py, protocol.py
│   │                 # and client.py, written by SDK team.
│   ├── docs          # SDK documentation source.
│   └── examples      # Example code for users to learn to use the SDK.
├── tests             # Scripts for running pytest against the current sdk module.
├── victor-proto      # Protobuf files submoduled from victor-proto repo.
│                     # (DO NOT MODIFY victor-proto DIRECTLY)
├── configure_dev.py  # Script to set up the SDK with your Vector (developer account).
└── update.py         # Use protoc to update the generated protobuf code.
                      # This copies and modifies the protobuf files from victor-proto.
```

# Victor SDK-related submodules and subtrees

In the victor repo there are the following subtrees and a submodule that is related to the Vector Python SDK:

```bash
.../victor
├──victor-clad                         # Subtree of victor-clad repo. Used for BLE/switchboard.
└──tools
    ├── protobuf/gateway               # Subtree of victor-proto. Original files are located here
    │                                  # in the victor repo. Update victor-proto repo with tools/protobuf/push_subtree.sh
    │                                  # See https://github.com/anki/victor-proto
    └── sdk/vector-python-sdk-private  # SDK files submoduled from the vector-python-sdk-private repo.
```

## Getting Started

Before developing with the SDK, you will need to make sure you have everything installed.

1. Install `homebrew`:

    ```bash
    # skip this if you already have homebrew
    /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    ```

1. Install `python3`:

    ```bash
    brew install python
    ```

1. `pip install` requirements:

    ```bash
    python3 -m pip install -r requirements.txt
    ```

1. Sync the `victor-proto` submodule:

    ```bash
    git submodule update --init --recursive
    ```

1. Update the protobuf generated code:

    ```bash
    python3 update.py
    ```

1. Install the SDK.
    Note: When developing the SDK, it's highly recommended you install `anki_vector` as an egg (using `-e`):

    ```bash
    python3 -m pip install -e ./sdk[3dviewer,docs,test]
    ```

1. Configure the SDK.

    Using a prod account:

    ```bash
    python3 -m anki_vector.configure
    ```

    Using a dev account:

    ```bash
    python3 configure_dev.py
    ```

## Using the SDK

For general usage, follow the instructions in the [sdk](sdk/README.md) directory.

## Release the SDK

When ready to release the SDK, make sure to notify the proper slack channels and get approval to release to the public.
Then follow the instructions in the [release-tools](release-tools/README.md) directory.

## Update proto files

The process to update the proto files starting from the originals is as follows:

1. Modify the original files in the victor repo in tools/protobuf/gateway/public, open PR and merge to victor master.

1. Sync to tip of victor master, then update victor-proto repo using script in the victor repo: tools/protobuf/push_subtree.sh (note that this currently auto-commits to master of victor-proto repo).

1. Update the victor-proto submodule in the vector-python-sdk-private repo, update the local proto files and docs, open PR and and merge to master:

    ```bash
    cd victor-proto
    git pull https://github.com/anki/victor-proto master
    cd ..
    ./update.py
    ```

When updating the victor-photo submodule, you may receive a message about SSO that starts like this:
`The 'anki' organization has enabled or enforced SAML SSO. To access this repository, you must use the HTTPS remote with a personal access token...`

In this case, it may help to use SSH instead:

 ```bash
git pull git@github.com:anki/victor-proto.git master
 ```

## Running `pytest` Tests

### Setup

1. Install the python sdk and test_requirements

    ```bash
    python3 -m pip install -r requirements.txt
    python3 -m pip install -e ./sdk
    ```

1. Set ANKI_ROBOT_SERIAL env variable to the robot you want to use (e.g., use "Local" for webots).

### Run all the `pytest` tests

From the root of this repo, run:

```bash
python3 -m pytest
```

### Run a single file's tests

From the root of this repo, run:

```bash
python3 -m pytest tests/test_anim.py
```

Or, for all the following tests, they can be ran directly from the tests directory, omitting `tests/`. Like so:

```bash
python3 -m pytest test_anim.py
```

### Run a single test function

From the root of this repo, run:

```bash
python3 -m pytest tests/test_anim.py::test_play_animation
```

Where `test_play_animation` is the name of the function to be tested.

### Run an individual test

From the root of this repo, run:

```bash
python3 -m pytest tests/test_anim.py::test_play_animation[sync]
```

Where `sync` is the `id` of the type of robot used for the test (see `tests/__init__.py` for possible ids).

### Notes

* For code coverage information, add `--cov=sdk/anki_vector` to get additional details about what % of code is exercised by the given tests.
* The pytest tests will be ran against the latest and all released versions of the SDK when using the [tests/all_versions.py](#Test-Multiple-Versions) script.

## Running Sample Code Tests

1. Make sure the SDK doc tests will run on your desired robot. For instance, you could set the default Robot constructor serial parameter to "Local" to test on webots.

1. Ensure your local build of the SDK is installed:

    ```bash
    python3 -m pip uninstall anki_vector
    cd vector-python-sdk-private/sdk
    python3 -m pip install -e .
    ```

1. Move robot off the charger.

1. Run the tests:

    ```bash
    cd docs
    make clean
    make html
    make doctest | tee output_file.txt
    ```

### Notes

* The sample code tests currently requires observing the results and deciding if the sample code requires fixing.

* There are sample code snippets that print output, but in our current setup, this results in the sample code "failing" because the expected output is `None`. Either ignore or come up with a plan to fix this overall.

* Unfortunately currently running all tests at once is not workable. One way to test them is to run them file by file, turning the sample code off in other files by changing `.. testcode::` to .. code-block:: python`.

## Test Multiple Versions

To test the functionality of all versions of the SDK, there is a script named `tests/all_versions.py`.
This script will use a virtual environment to install each released version of the SDK, and the currently developed version of the SDK against a given robot deployment (usually latest).

### Running Against Live Robot

With ANKI_ROBOT_SERIAL set to your robot's serial number, run the script.

```bash
python3 tests/all_versions.py
```

The script will do the following:

* Create a virtualenv

* Install a version of anki_vector into the virtualenv

* Download all the dependencies of the version

* Run the pytests

The resulting output should look something like:

```bash
[0.5.1] > virtualenv -p /usr/local/opt/python/bin/python3.6 ./tests/env --no-site-packages
[0.5.1] > ./tests/env/bin/python -m pip install -r requirements.txt
[0.5.1] > ./tests/env/bin/python -m pip install anki_vector[3dviewer,docs,test]==0.5.1
[0.5.1] > ./tests/env/bin/python -m pytest
................ss............................................xxxx.......................................................................... [100%]
134 passed, 2 skipped, 4 xfailed in 277.29 seconds
[latest] > virtualenv -p /usr/local/opt/python/bin/python3.6 ./tests/env --no-site-packages
[latest] > ./tests/env/bin/python -m pip install -r requirements.txt
[latest] > ./tests/env/bin/python -m pip install ./sdk[3dviewer,docs,test]
[latest] > ./tests/env/bin/python -m pytest
..............................................................ssss.......................................................................... [100%]
136 passed, 4 skipped in 272.76 seconds
```

### Running against webots

To run against a simulated robot use:

```bash
python3 tests/all_versions.py -s local
```

## Test grpc

See `tools/sdk/gateway-tests/README.md` in the victor repo for instructions to run `test_json_rpcs.py` and `test_json_streaming.py`.

## Run Python Linter

1. After you make Python changes, run the Python linter: 

    ```bash
    cd release-tools
    ./lint.sh
    ```
