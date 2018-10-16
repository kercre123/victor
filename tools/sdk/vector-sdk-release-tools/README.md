# Vector Python SDK Build Tools

To release a new version of the SDK, follow the steps below.

These instructions build the SDK and its documentation and produce a compressed file.


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

3. On victor master branch, delete `victor/tools/sdk/vector-sdk/tests` folder as we don't want to ship this folder.
    cd victor
    rm -rf tools/sdk/vector-sdk/tests

4. On victor master branch, remove proto messages that we don't want to expose. Do not commit these changes.

  a. From `cloud/gateway/external_interface.proto`, remove:
        `UploadDebugLogs`
        `CheckCloudConnection`
        `UpdateUserEntitlements`

  b. From `cloud/gateway/messages.proto`, remove:
        `UploadDebugLogsRequest`
        `UploadDebugLogsResponse`
        `CheckCloudRequest`
        `CheckCloudResponse`

  c. From `cloud/gateway/settings.proto`, remove:
        `enum UserEntitlement`
        `message UserEntitlementsConfig`
        `UpdateUserEntitlementsRequest`
        `UpdateUserEntitlementsResponse`

  d. From `cloud/gateway/shared.proto`, remove:
        `UpdateUserEntitlementsRequest`
        `UpdateUserEntitlementsResponse`
        `CheckCloudRequest`
        `CheckCloudResponse`

5. Run `victor/tools/sdk/scripts/update_proto.sh`

6. Pull latest `vector-python-sdk-private` master branch.

7. Create a branch off master in`vector-python-sdk-private` named `release/{version}`.

8. Delete all local files from `vector-python-sdk-private`. Do not revert.

9. Copy contents of `victor/tools/sdk/vector-sdk/` to `vector-python-sdk-private`.

10. Locally delete `anki_vector/messaging/.gitignore` from repo `vector-python-sdk-private`

11. In `vector-python-sdk-private` repo:
    ** update version in `anki_vector/version.py`
    ** Update version throughout docs, like on Mac, Win and Linux install pages.

12. In `vector-python-sdk-private` repo, navigate to `docs` and run: `make html`.

13. Use the diff of all files in the repo to help collect release notes if you don't have them already.

14. Copy contents of `vector-python-sdk-private` repo to new folder outside of your repo named `vector_python_sdk_{version}` (e.g., vector_python_sdk_0.4.0).
    * Do not copy over .git folder.

15. Compress folder `vector_python_sdk_{version}` repo into `.zip` and `.tar` by running the following commands:
      `zip -r vector_python_sdk_{version}.zip vector_python_sdk_{version}`
      `tar -zcvf vector_python_sdk_{version}.tar.gz vector_python_sdk_{version}`


# Test the Build

Test the compressed files.

1. Uncompress and open documentation:

```bash
$ cd vector_python_sdk_{version}
$ open docs/build/html/index.html
```

2. Test SDK Installation and Vector Authentication following the instructions in the docs, ideally including running configure.py with an MP robot (or `configure_dev.py` from `victor/tools/sdk` if necessary).

3. Test all tutorials and apps (excluding sign language).


# Push the Release Commit To Master and Tag

In `vector-python-sdk-private` repo, validate reasonable diff, commit and push.

Add tag in `vector-python-sdk-private` repo against tip (which represents release) with version number.
$ git tag -a {version} -m "Release of {version}"
$ git push origin master --tags
