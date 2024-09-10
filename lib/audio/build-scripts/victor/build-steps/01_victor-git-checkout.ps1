# ---------------------------------------
### Variables

$anki_git_repos_url = "github.com/anki"

$git_dir = ".git"

$workspace_dir = $env:PROJECT_WORKSPACE # "AnkiGithub" # The workspace directory that will handle our repo.

$git_username = $env:GIT_USERNAME # "anki-smartling"
$git_ssh_token = $env:GIT_SSH_TOKEN
$git_remote = $env:GIT_REMOTE # "origin"
$git_branch = $env:GIT_BRANCH # "master"
$git_remote_branch = "${git_remote}/${git_branch}"
# $git_commit_hash = $env:GIT_COMMIT_HASH # will work like SVN revision (?)
$git_repo_name = $env:GIT_REPO_NAME # "victor-wwise"
$git_repo_url = "${anki_git_repos_url}/${git_repo_name}.git"

$slack_channel = $env:SLACK_CHANNEL
$slack_username = $env:SLACK_USERNAME # "buildbot" # The username of the Slack Bot.


# ---------------------------------------
### Logging

echo "INFO - env:GIT_REPO_NAME`: $git_repo_name"
echo "INFO - env.GIT_BRANCH`: $git_branch"
echo "INFO - env.SLACK_CHANNEL`: $slack_channel"


# ---------------------------------------
### Functions

# Sends a failure message to Slack channel
function SendFailureSlackMessage([String] $error_message) {
  $payload = @{
      channel = $slack_channel;
      username = $slack_username;
      attachments = @(
          @{
          text = $error_message;
          fallback = $error_message;
          color = "danger";
          };
      );
  }
  Invoke-Restmethod -Method POST -Body ($payload | ConvertTo-Json -Depth 4) -Uri $env:SLACK_TOKEN_URL
}

# Checks exit code and maybe send a failure message to Slack
function CheckExitCode([Int] $exit_code, [String] $error_message) {
    if ($exit_code -ne 0) {
      echo "INFO - Exiting script. $error_message Build failed. exitCode $exit_code"
      SendFailureSlackMessage "$error_message Build failed. exitCode $exit_code"
      exit $exit_code
    }
}

# Performs 'git clone' with Github URL, username, and SSH key.
function CloneGitRepo() {
  git clone -b ${git_branch} https://${git_username}:${git_ssh_token}@${git_repo_url}
  CheckExitCode $lastExitCode "git clone failed"
  Push-Location $git_repo_name
  git lfs fetch --all
  CheckExitCode $lastExitCode "git lfs fetch all failed"
  git lfs checkout
  CheckExitCode $lastExitCode "git lfs checkout failed"
  Pop-Location
}

# Similar to a 'git pull', but 'fetches' the latest repo and 'resets' the local repo to that latest
# repo. Using this method because 'git pull' is literally a 'git fetch' followed by a 'git merge'.
# Merging conflicts can occur from 'git pull' and forcibly setting the local repo to the latest
# remote one is a fine choice in this case.
function CheckoutGitRepo() {
  git fetch --all
  CheckExitCode $lastExitCode "git fetch all failed"
  git reset --hard ${git_remote_branch}
  CheckExitCode $lastExitCode "git reset hard failed"
  git lfs fetch --all
  CheckExitCode $lastExitCode "git lfs fetch all failed"
  git lfs checkout
  CheckExitCode $lastExitCode "git lfs checkout failed"
}

# Performs 'git pull' to update the repo. If the repo doesn't exist, then performs 'git clone'
function UpdateRepo() {
  # Wherever the Windows Remote starts running this command, check for the workspace directory $workspace_dir
  # If workspace directory doesn't exists, make that directory and clone.
  if (!(Test-Path $workspace_dir)) {
    New-Item -Path $workspace_dir -ItemType directory
  }
  Push-Location $workspace_dir
  if (!(Test-Path $git_repo_name)) {
    CloneGitRepo
  }
  else {
    # Checks if the '.git' directory exists. If so, checkout the latest git repo,
    # else reclone the entire repo again.
    Push-Location $git_repo_name
    if (!(Test-Path $git_dir)) {
      Push-Location ..
      Remove-Item -Recurse -Force $git_repo_name
      CloneGitRepo
      Pop-Location
    }
    else {
      CheckoutGitRepo
    }
    Pop-Location
  }
  Pop-Location
}


# ---------------------------------------
### Main

echo "INFO - Getting latest from https://${git_repo_url}"
UpdateRepo


