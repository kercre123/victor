# Vector Python SDK Build Tools

To release a new version of the SDK, follow the steps below.

These instructions build the SDK and its documentation and produce a compressed zip and tar file.


# Pre-Requisites

* You have ota'd your robot to Victor OS 1.0.1 or later. (TODO: test multiple OS versions? Test latest OS?)
* Uninstall any existing representation of the SDK from your computer.
* You have Python 3.6.1 or later already installed.
* Decide what version you want to increase the Python SDK to.


# Build the SDK Artifacts

1. Pull latest victor repo's master branch.

2. Delete all local files from `victor/tools/sdk/vector-sdk/` and `victor/cloud/gateway/` by wiping dirs then reverting them entirely:
    cd victor
    git checkout -- tools/sdk/vector-sdk/
    git clean -f -d -x tools/sdk/vector-sdk/
    git checkout -- cloud/gateway/
    git clean -f -d -x cloud/gateway/

3. On victor master branch, from the following files, remove all messages marked "App only". This removes proto messages that we don't want to expose. Do not commit these changes to victor master.

  `cloud/gateway/external_interface.proto` (Keep the ending curly brace)
  `cloud/gateway/messages.proto`
  `cloud/gateway/onboardingSteps.proto`
  `cloud/gateway/settings.proto`
  `cloud/gateway/shared.proto`

4. Run `victor/tools/sdk/scripts/update_proto.sh`. Be sure there are no warnings or errors before continuing.

5. Pull latest `vector-python-sdk-private` master branch.

6. Create a branch off master in`vector-python-sdk-private` named `release/{version}`.

7. Delete all local files from `vector-python-sdk-private`. Do not revert.

8. Copy contents of `victor/tools/sdk/vector-sdk/` to `vector-python-sdk-private`.

9. Locally delete `anki_vector/messaging/.gitignore` from repo `vector-python-sdk-private`

10. In `vector-python-sdk-private` repo:
    ** update version in `anki_vector/version.py`
    ** Update version throughout docs, like on Mac, Win and Linux install pages.

11. In `vector-python-sdk-private` repo, navigate to `docs` and run: `make html`.

12. Delete any __pycache__ folders.

13. Delete all `.DS_Store` files from the `vector-python-sdk-private` repo:
      `cd vector-python-sdk-private`
      `find . -name '.DS_Store' -type f -delete`

14. Use the diff of all files in the repo to help collect release notes if you don't have them already.

15. Create zip and tar files (e.g. vector_python_sdk_0.4.0.zip and vector_python_sdk_0.4.0.tar):
      `cd vector-python-sdk-private`
      `zip -r ~/Desktop/vector_python_sdk_{version}.zip *`
      `tar -zcvf ~/Desktop/vector_python_sdk_{version}.tar.gz *`

16. Create boss file, review it and commit it.
  a. Unzip the zip file
  b. cd inside the zip file
  c. Run: `find . -type f > ../boss.txt`
  d. Compare the new boss.txt file to this file in the victor repo: tools/sdk/vector-sdk-release-tools/boss.txt
  e. If files have been added that shouldn't be there, remove the files and repeat the step to create the compressed files.
  f. Commit boss.txt to the victor repo if it has changes we want to keep.


# Test the Build

Test the compressed files.

1. Uncompress and open documentation:

```bash
$ cd vector_python_sdk_{version}
$ open docs/build/html/index.html
```

2. Test SDK Installation and Vector Authentication following the instructions in the docs, ideally including running configure.py with an MP robot.

3. Test all tutorials and apps.


# Push the Release Commit To Master and Tag

In `vector-python-sdk-private` repo, validate reasonable diff, commit and push.

Add tag in `vector-python-sdk-private` repo against tip (which represents release) with version number.
$ git tag -a {version} -m "Release of {version}"
$ git push origin master --tags
