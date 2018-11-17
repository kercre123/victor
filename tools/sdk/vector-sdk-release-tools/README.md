# Vector Python SDK Build Tools

To release a new version of the SDK, follow the steps below.

These instructions build the SDK and its documentation and produce a compressed zip and tar file.


# Pre-Requisites

* You have ota'd your robot to the version of the OS you plan to test with. (Consider: test with multiple versions?)
* Uninstall any existing representation of the SDK from your computer.
* You have Python 3.6.1 or later already installed.
* Decide what version you want to increase the Python SDK to.


# Build the SDK Artifacts

1. Pull latest victor repo's master branch.

1. Delete all local files from `victor/tools/sdk/vector-sdk/` and `victor/cloud/gateway/` by wiping dirs then reverting them entirely:

    ```bash
    cd victor
    git checkout -- tools/sdk/vector-sdk/
    git clean -f -d -x tools/sdk/vector-sdk/
    git checkout -- cloud/gateway/
    git clean -f -d -x cloud/gateway/
    ```

1. On victor master branch, from the following files, remove all messages marked "App only". This removes proto messages that we don't want to expose. Do not commit these changes to victor master.

    * `cloud/gateway/alexa.proto`
    * `cloud/gateway/external_interface.proto` (Keep the ending curly brace)
    * `cloud/gateway/messages.proto`
    * `cloud/gateway/onboardingSteps.proto`
    * `cloud/gateway/settings.proto`
    * `cloud/gateway/shared.proto`

    In addition, take care to review and not expose any new endpoints that we don't want exposed from proto files in `cloud/gateway`.

1. Run `victor/tools/sdk/scripts/update_proto.sh`. Be sure there are no warnings or errors before continuing.

1. Pull latest `vector-python-sdk-private` master branch.

1. Create a branch off master in`vector-python-sdk-private` named `release/{version}`.

1. Delete all local files from `vector-python-sdk-private`. Do not revert.

1. Copy contents of `victor/tools/sdk/vector-sdk/` to `vector-python-sdk-private`.

1. Locally delete `anki_vector/messaging/.gitignore` from repo `vector-python-sdk-private`

1. In `vector-python-sdk-private` repo:

    * update version in `anki_vector/version.py`
    * Update version throughout docs, like on Mac, Win and Linux install pages.

1. In `vector-python-sdk-private` repo, navigate to `docs` and run: `make html`.

1. Delete all `.DS_Store` files from the `vector-python-sdk-private` repo:

    ```bash
    cd vector-python-sdk-private
    find . -name '.DS_Store' -type f -delete
    ```

1. Delete any __pycache__ folders:

    ```bash
    cd vector-python-sdk-private
    find . -name  '__pycache__' -exec rm -rf {} \;
    ```

1. Use the diff of all files in the repo to help collect release notes if you don't have them already.

1. Create zip and tar files (e.g. vector_python_sdk_0.4.0.zip and vector_python_sdk_0.4.0.tar.gz):

    ```bash
    cd vector-python-sdk-private
    zip -r ~/Desktop/vector_python_sdk_{version}.zip *
    tar -zcvf ~/Desktop/vector_python_sdk_{version}.tar.gz *
    ```

1. Create distribution files (e.g. anki_vector-0.4.0-py3-none-any.whl and anki_vector_0.4.0.tar.gz):

    ```bash
    cd vector-python-sdk-private
    python3 setup.py sdist bdist_wheel
    ```

    The files will appear under `vector-python-sdk-private/dist`

1. Create boss file, review it and commit it.

    1. Unzip the zip file
    1. cd inside the zip file
    1. Run: `find . -type f > ../boss.txt`
    1. Compare the new boss.txt file to this file in the victor repo: tools/sdk/vector-sdk-release-tools/boss.txt
    1. If files have been added that shouldn't be there, remove the files and repeat the step to create the compressed files.
    1. Commit boss.txt to the victor repo if it has changes we want to keep.


# Test the Build

Test the compressed files.

1. Uncompress and open documentation:

    ```bash
    $ cd vector_python_sdk_{version}
    $ open docs/build/html/index.html
    ```

1. Test SDK Installation and Vector Authentication following the instructions in the docs, ideally including running configure.py with an MP robot.

1. Test all tutorials and apps.


# Push the Release Commit To Master and Tag

In `vector-python-sdk-private` repo, validate reasonable diff, commit and push.

Add tag in `vector-python-sdk-private` repo against tip (which represents release) with version number.

```bash
$ git tag -a {version} -m "Release of {version}"
$ git push origin master --tags
```

# Deploy to PyPi

1. Verify the following about your setup:

    * `twine` is installed. If not, use:

        ```bash
        python3 -m pip install --user twine
        ```

    * You have access to the `Cozmo SDK Build Secrets` in 1Password (yes _Cozmo_). If not, contact IT and ask for permissions.

1. Upload the package using the `PyPi Developer Login` from 1 Password:

    ```bash
    python3 -m twine upload dist/*
    ```

1. Verify the deployment worked:

    * Navigate to [anki_vector's pypi webpage](https://pypi.org/project/anki-vector/) and check that the version uploaded.
    * Install `anki_vector` with pip and run the tests

        ```bash
        python3 -m pip uninstall anki_vector
        python3 -m pip install anki_vector
        ```
