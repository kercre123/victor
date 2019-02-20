# DEPS (dependencies)

To build Victor, we rely on a number of 3rd party dependencies and
build artifacts from other projects.  We keep track of most of those
dependencies in the top-level `DEPS` file.

## Dependencies from deptool

### How to add a new deptool dependency

Let's assume you have decided to add a new open source project called
`whizbang`.  It has a suitable open source license that has been
approved by the legal department.  The code is available on github
or some other public site.  You have done some testing with the code
from tag `v1.4` and it works well.  Do the following:

1.  Write a build script for `whizbang`.  You will likely need to
produce builds for mac and vicos.  There are numerous examples to
be found in https://github.com/anki/build-tools/tree/master/deps/victor.
```
$ git clone git@github.com:anki/build-tools.git
$ cd build-tools
$ git checkout -b add-whizbang-build-script --track origin/master
$ emacs -nw deps/victor/build-whizbang.sh
```
As you will see in the other example scripts, like `build-aubio.sh`, you
should reference an environment variable like `WHIZBANG_REVISION_TO_BUILD`, for
deciding which git commit sha or tag you check out and build.  If that
variable is unset, your script you should give an error message and exit.

2. Create a build for `whizbang` on TeamCity under the
[Dependencies/Victor](https://build.ankicore.com/project.html?projectId=Dependencies_Victor&tab=projectOverview)
project.  Follow the same pattern as the other projects and name the
build configuration `whizbang`.  Base it on the
`Build Victor Dependency Template`.  Add an environment variable parameter like
`WHIZBANG_REVISION_TO_BUILD` with the current git commit sha or tag that you will
want to build.  This build template has 2 steps.  The first calls your build
script and the second uploads artifacts to S3.  If you want, you can test your
build without uploading to S3 by disabling step 2.  If you get stuck, ask for help.
3. Merge your PR for the `build-tools` repo that you created above.
4. Run the `whizbang` build on TeamCity and the `whizbang-v1.4.tar.bz2` and
`whizbang-v1.4-SHA-256.txt` artifacts will be uploaded to S3.
5. Take note in the build log of the SHA256 hash of your artifact.
6. Add an entry to the `deptool` section of the `DEPS` file for `whizbang`:
```
"whizbang": {
    "checksums": {
        "sha256": "<the full sha256 hash>"
    },
    "version": "v1.4"
}
```
7. The next time you run a build, a symlink for `whizbang` will be
created under the `EXTERNALS` directory pointing to
`~/.anki/deps/victor/whizbang/dist/v1.4`

### How to remove a deptool dependency

If you are no longer using a dependency, please remove it from `DEPS`.
1. Remove all code that references the dependency's headers and
libs.
2. Remove references in the documentation about the dependency.
3. Go into `DEPS` and remove the entry.
4. Do test builds to make sure everything still works.

### How to upgrade / downgrade a deptool dependency

Following the above example, let's assume that `whizbang` `v1.5` comes
out and you decide that we should upgrade to it.  Do the following:

1. Go to the build on TeamCity and update the `WHIZBANG_REVISION_TO_BUILD`
parameter to `v1.5`.
2. Run a new build and `whizbang-v1.5.tar.bz2` will be uploaded to S3
and give you a new SHA256.
3. Go to the `DEPS` file and update the `version` field to `v1.5`
and the `sha256` field to the new SHA256 value.
4. Write up a commit message that explains why you upgraded to `v1.5`
from `v1.4`.

## Dependencies from Artifactory

If an existing project uploads its artifacts to Artifactory, you can
add it as a dependency in the DEPS file under the `artifactory` section.
Follow the existing pattern and be sure to add the SHA256 checksum.  Currently,
the Artifactory dependencies are downloaded from a server that is only
accessible on the local SF office LAN or through VPN.  To facilitate remote
usage, these artifacts are cached locally under `~/.anki/deps-cache/sha256`
by their SHA256 checksum name.  If you have downloaded them once, you do not
have to download them again, even if you clean your workspace.

## Dependencies from SVN

Please do not add any new SVN dependencies.  We would like to move off
SVN as soon as possible.  Like the artifactory dependencies, the SVN server
is only accessible via LAN or VPN.  As well, these SVN checkouts are cached
locally under `~/.anki/deps-cache/svn` to facilitate remote
usage and to minimize the need to re-download them after a clean.

## Dependencies from TeamCity

Currently, we are not using any dependencies directly from TeamCity.  However,
we have left in place the credentials and URL for accessing these
artifacts if needed.  Like Artifactory and SVN, the TeamCity artifacts are
only accessible on the local SF office LAN or through VPN.  Like the Artifactory
dependencies, these dependencies are cached locally under `~/.anki/deps-cache/sha256`
by their SHA256 checksum name.

## Dependencies from files

Please do not add any new dependencies to the `files` section. At the time of
this writing, there is 1 file dependency given by a URL that is only accessible
on the local SF office LAN or through VPN.  These dependencies are also
cached locally under `~/.anki/deps-caches/files`.