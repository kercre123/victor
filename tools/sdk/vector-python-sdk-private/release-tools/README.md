# Vector Python SDK Build Tools

The scripts stored here will checkout, build and upload the SDK and its
documentation to PyPi and S3.

To release a new version of the SDK, follow the steps below.

## Pre-Requisites

* You have ota'd your robot to the version of the OS you plan to test with. (Consider: test with multiple versions?)
* You have 1Password installed from Anki and you have access to the "Cozmo SDK Build Secrets"
  1Password Vault (yes, _Cozmo_). If not, make a request to IT for both. Includes the following passwords:
  * Cozmo SDK Docs AWS S3 bucket
  * PyPI Developer Login
* You have Python 3.6.1 or later already installed.
    ```bash
    python3 --version
    ```
* Merge any updates made directly to `vector-python-sdk` into `vector-python-sdk-private`.
    ```bash
    # run this from the root of the private repo
    git subtree pull --prefix sdk/ git@github.com:anki/vector-python-sdk.git master
    ```
* Confirm lint runs cleanly (and update code if not):
    ```bash
    ./release-tools/lint.sh
    ```
* Confirm proto files are up-to-date. Run the update script at the root of this repository to generate the protobuf code.
    ```bash
    python3 update.py
    ```

    Take care to review the proto files in `sdk/anki_vector/messages` and not expose any new endpoints or messages that we don't plan to add.

    Check whether there are diffs to commit and commit changes.
* You have tested all examples (tutorials and apps). Grab the private repo master branch and ensure you are at tip. Install the latest SDK candidate code:
    ```bash
    cd sdk
    python3 -m pip uninstall anki_vector
    python3 -m pip install -e .
    ```
* Confirm that nightly tests are passing with the internal SDK build
    * Run [tests/all_versions.py](https://github.com/anki/vector-python-sdk-private/blob/master/README.md#Test-Multiple-Versions) as directed and see that it passes.
    * Run these tests in the victor repo and confirm they pass:
        ```bash
        cd tools/sdk/vector-sdk-tests/automated
        ./test_all_messages.py
        ./test_gateway.py
        ```
* Confirm that Sphinx docs build cleaning without warnings or errors.
    ```bash
    cd sdk/docs
    make clean
    make html
    ```
* Uninstall any existing representation of the SDK from your computer.
    ```bash
    python3 -m pip uninstall anki_vector
    ```
* Decide what version you want to increase the Python SDK to.

## Source the `activate` Script

To automatically set up all the environment variables for later scripts, source 
the activate script, and follow the prompts. Fetch the twine password from 1Password
("PyPi Cozmo Login" at https://anki.1password.com )

Versions formatting should follow: `MajorVersion.MinorVersion.Patch`

Run the following:

```bash
cd vector-python-sdk-private
source release-tools/activate
```

## Push the private subtree to the public repo

1. Check that you are on master at tip of vector-python-sdk-private repo.

1. Push the sdk subtree to the public repo:

    ```bash
    python3 release-tools/deploy_public.py -b release/$SDK_VERSION
    ```

1. Open a pull request in the public repo, and have another SDK engineer approve it.

1. Merge your PR into master (via commit merge NOT squash and merge, we want the commit history in this case!)

1. Use the diff to help collect release notes if you don't have them already. This includes proto changes, where even type changes can affect SDK users. Also check the commit history of master for any other changes since the previous release.

## Build the SDK Artifacts

Run the `build-release.sh` script

```bash
./release-tools/build-release.sh
```

This will automatically perform the following steps:
1. Check out the public python sdk repo into a temporary directory
1. Set up a virtualenv environment in another temporary directory and installs
   the required build tools (sphinx, numpy, etc) into it
1. Create a branch prefixed with `build_master` in the public repo and create a release commit
   within it, that holds the changes to `sdk/anki_vector/version.py`
   that will ultimately be pushed into the master branch
1. Create a branch in the public repo prefixed with `build_local` that makes the same updates
   but with the version set to the actual release version. This branch will NOT be actually
   pushed to the repo.
1. Run `make dist` to build the artifacts in the dist folder, including
   anki_vector_sdk_examples_$SDK_VERSION.tar.gz, anki_vector_sdk_examples_$SDK_VERSION.zip,
   and anki_vector-*-py3-none-any.whl.
1. Run Sphinx to generate the documentation and copies it into `dist/html`

Once this script completes, in the next step you will cd into the repo directory it has created
(printed as the final line of the script output) to continue.

## Test the Build

1. Change into the repo directory created above, and open the documentation
    in your local browser to check for correctness. Be sure to check that you
    can search the API docs.

    ```bash
    cd /private/var/folders/4n/nh2dz8js47g5dy3tqhsxlxs40000gp/T/tmp.8kiHzxSG/vector-python-sdk
    open dist/html/index.html
    ```

1. Test the anki_vector package generated in the dist folder following the instructions in the docs.

    ```bash
    cd /private/var/folders/4n/nh2dz8js47g5dy3tqhsxlxs40000gp/T/tmp.8kiHzxSG/vector-python-sdk/dist/
    pip3 install anki_vector-$SDK_VERSION-py3-none-any.whl
    ```

1. Test running the anki_vector.configure executable submodule with a robot using prod endpoints. Note that you may need to unset env variables such as ANKI_ROBOT_HOST.

1. Spot check tutorials and apps.

## Deploy anki_vector package to PyPI

1. Upload the package using the `PyPI Developer Login` from 1Password:

    ```bash
    python3 -m pip install --user twine
    python3 -m twine upload -r pypi -u $TWINE_USER -p $TWINE_PASSWORD dist/anki_vector-$SDK_VERSION-py3-none-any.whl
    ```

1. Verify the deployment worked:

    * Navigate to [anki_vector's PyPI webpage](https://pypi.org/project/anki-vector/) and check that the version uploaded.
    * Install `anki_vector` with pip and run the tests

        ```bash
        python3 -m pip uninstall anki_vector
        python3 -m pip install anki_vector
        ```

## Publish the Documentation

You must have the aws cli tool available and configured. Run these commands if your computer hasn't already been set up:

```bash
brew install awscli
aws configure
```

You will need the "Cozmo SDK Docs AWS Key" from the 1Password "Cozmo SDK Build Secrets"
referred to in Pre-Requisites above.

Run these commands to upload the website documentation files:

```bash
cd /private/var/folders/4n/nh2dz8js47g5dy3tqhsxlxs40000gp/T/tmp.8kiHzxSG/vector-python-sdk/dist/
aws s3 sync --cache-control no-cache html/ s3://$S3_BUCKET/vector/docs/
```

Run these commands to upload the compressed example files:

```bash
cd /private/var/folders/4n/nh2dz8js47g5dy3tqhsxlxs40000gp/T/tmp.8kiHzxSG/vector-python-sdk/dist/
aws s3 sync --cache-control no-cache --exclude '*' --include "anki_vector_sdk_examples_$SDK_VERSION.tar.gz"  --include "anki_vector_sdk_examples_$SDK_VERSION.zip" . s3://$S3_BUCKET/vector/$SDK_VERSION/
```

Check that both the docs and linked examples published correctly.

## In public repo, push the release commit to master and tag repo.

The script above will have created a release commit for you,
which can just be rebased into master, tagged, and pushed to origin.

Note that the diff will show that master will be updated with the next dev version (e.g., "0.5.2.dev0").

    ```bash
    git branch
      build_local_1.1.0
    * build_master_1.1.0
      master

    git checkout master
    git rebase build_master_$SDK_VERSION
    git show HEAD
      (validate reasonable diff)

    git tag -a $SDK_VERSION -m "Release of $SDK_VERSION"
    git push origin master --tags
    ```

## Tag private repo

1. Pull changes made to public repo during the release process:

    ```bash
    # run this from the root of the private repo
    git subtree pull --prefix sdk/ git@github.com:anki/vector-python-sdk.git master
    ```

1. Add tag in `vector-python-sdk-private` repo against tip (which represents release) with version number.

    ```bash
    cd vector-python-sdk-private
    git tag -a $SDK_VERSION -m "Release of $SDK_VERSION"
    git push origin master --tags
    ```

## Add newest version to tests/all_versions.py

In `tests/all_versions.py` there is a list of versions to be tested named `VERSIONS`.
Add the newly deployed version to that list to be tested for backwards compatibility.

## Deactivate the Sourced Script

```bash
deactivate
```

## Pour A Beer

You're done.
