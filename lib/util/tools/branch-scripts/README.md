
# ANKI BRANCH SCRIPTS

## USAGE

These scripts should run within a "clean" workspace. Any local changes will interfere with checkout and merge operations.

```bash
  #!/usr/bin/env bash
  victor-branch-diff.sh
```

List commits made to master branch that have not been merged to release branch.
Branch names can be customized in the bash script.

```bash
  #!/usr/bin/env bash
  victor-branch-cherrypick.sh [picklist]
```

For each SHA in `picklist`, perform `git cherry-pick` from master to release branch.
Cherry-picks will be committed to your repo but will NOT be automatically pushed to github.
You should review all changes for correctness before you push to github.

Branch names can be customized in the bash script.
If a SHA has already been picked, that's OK.

```bash
  #!/usr/bin/env bash
  victor-branch-update-spreadsheet.sh
```

Update Google Docs release spreadsheet to show commits made to master branch that have not been merged
to release branch. Spreadsheet, sheet name, and branch names can be customized in the bash script.

New rows are inserted below row 3, leaving room for title and spacing row at top.
Formatting will be copied from the point of insertion.
If a row has been moved down to "APPROVED" or "DENIED", it will not be added again.
Existing rows will never be modified, so any manual edits will be preserved.

Rows that have been deleted may reappear on a later run.
Use an exclude list (eg `./excludes` to prevent a row from reappearing.

## GIT AUTHORIZATION

If you are using GIT with SSH authentication, you may be prompted to
enter the passphrase for your SSH key. This is required to perform
push and pull operations to github.

If you add your SSH identity to your keychain (e.g. `ssh-add`) you will
not be prompted to enter your passphrase.

## GOOGLE API AUTHORIZATION

`anki-branch-update-spreadsheet.py` requires a secret application key to access the Google Sheets API.

Download the application key from Google Developer Console:

<https://console.developers.google.com/apis/credentials/oauthclient/825168049263-rcj6l2kgf5vj4desvcsnrr012tkrrcr4.apps.googleusercontent.com?project=valid-actor-178505>

Click the file_download (Download JSON) button to the right of the client ID.
Move this file to your script directory and rename it `client_secret.json`.

When you run `anki-branch-update-spreadsheet.py`, it will attempt to launch a browser window to complete authentication.
You must complete authentication to allow the client secret to be used with your Google ID.
An authorization token will be stored in your `~/.credentials` directory.

If you don't want to share permissions with other copies of this script, you can generate your own project and
set of credentials. When you run `anki-branch-update-spreadsheet.py`, it will attempt to use whatever client secret
is installed as `client_secret.json`.

If you change your client secret, you must remove any existing authorization token from your `~/.credentials`.

SEE ALSO

* <https://developers.google.com/sheets/api/quickstart/python>

* <https://developers.google.com/sheets/api/guides/authorizing>
