
$anki_git_repos_url = "github.com/anki"

$workspace_dir = $env:PROJECT_WORKSPACE
$git_repo_name = $env:GIT_REPO_NAME
$branch = $env:SVN_BRANCH
$revision = $env:SVN_REVISION
$build_number = $env:BUILD_NUMBER
$slack_channel = $env:SLACK_CHANNEL
$asset_svn_path = "victor-audio-assets"

echo "INFO - env:GIT_REPO_NAME`: $git_repo_name"
echo "INFO - env.SVN_BRANCH`: $branch"
echo "INFO - env.SVN_REVISION`: $revision"
echo "INFO - env.BUILD_NUMBER`: $build_number"
echo "INFO - env.SLACK_CHANNEL`: $slack_channel"
echo "INFO - audio asset repo: $asset_svn_path"


# send a failure message to slack channel
function SendFailureSlackMessage([String] $error_message) {
  $payload = @{
      channel = $slack_channel;
      username = "buildbot";
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

# send a success message to slack channel
function SendSuccessSlackMessage([String] $success_message, [String] $other) {
  $payload = @{
      text = $success_message;
      channel = $slack_channel;
      username = "buildbot";
      attachments = @(
          @{
          fallback = $success_message;
          color = "good";
          fields = @(
              @{
              title = "";
              value = $other;
              });
          };
      );
  }

  # send Slack message
  Invoke-Restmethod -Method POST -Body ($payload | ConvertTo-Json -Depth 4) -Uri $env:SLACK_TOKEN_URL
}

if ($revision -ne "HEAD") {
  #$revision_commit_log = svn log -r $revision https://svn.ankicore.com/svn/victor-wwise/$branch
  #$revision_commit_log = [string]::join("`n", $revision_commit_log)
  #$revision_commit_log = $revision_commit_log -replace "------------------------------------------------------------------------",""
  #$revision_commit_log = $revision_commit_log -replace '"',"'"

  # TODO: Look in Git rather than SVN for the commit log (VIC-14176)
  $revision_commit_log = "unknown"

  SendSuccessSlackMessage "No files were checked in.  This was a revision specific build.`nSVN branch: *$branch* `nTeamCity build: *$build_number*" $revision_commit_log
  echo "INFO - No files were checked in.  This was a revision specific build.`nSVN branch: *$branch* `nTeamCity build: *$build_number*`n$revision_commit_log"
  exit 0
}

# get last wwise revision
#$wwise_last_rev = svn log -l1 https://svn.ankicore.com/svn/victor-wwise/$branch | select-string -pattern 'r[0-9]'
#$wwise_last_rev = $wwise_last_rev.Line.Split('\|')[0] -replace 'r', ''
#$wwise_last_rev = $wwise_last_rev.trim() -as [int]

# TODO: Look in Git rather than SVN for the revision info (VIC-14176)
$wwise_last_rev = -1

echo "INFO - Wwise last revision on $branch`: $wwise_last_rev"

# get log range based on last revision of the wwise and soundbanks repos
$commit_search_pattern = ""
if ($branch -eq "trunk") {
  $commit_search_pattern = 'Victor Wwise Project Commit'
} Else {
  $commit_search_pattern = "Victor Wwise Project \($branch\) Commit"
}
$soundbanks_last_known_rev = (svn log -l20 https://svn.ankicore.com/svn/victor-audio-assets/trunk | select-string -pattern $commit_search_pattern)[0]
if ([string]::IsNullOrWhitespace($soundbanks_last_known_rev)) {
  echo "INFO - Soundbanks last known revision not found"
  $soundbanks_last_known_rev = $wwise_last_rev-1
} Else {
  if ($soundbanks_last_known_rev -like '*,*') {
    $soundbanks_last_known_rev = $soundbanks_last_known_rev.Line.Split(',')[-1] -as [int]
  } ElseIf ($soundbanks_last_known_rev -like '*-*') {
    $soundbanks_last_known_rev = $soundbanks_last_known_rev.Line.Split('-')[-1].trim() -as [int]
  } Else {
    $soundbanks_last_known_rev = $soundbanks_last_known_rev.Line.Split(' ')[-1].trim() -as [int]
  }
  echo "INFO - Soundbanks last known revision on $branch`: $soundbanks_last_known_rev"
}
$lowest_commit = $soundbanks_last_known_rev+1
$log_range = "$wwise_last_rev`:$lowest_commit"
echo "INFO - Log range for commit message: $log_range"

# verify there are new commits to add
$num_commits = $wwise_last_rev - $soundbanks_last_known_rev -as [int]
echo "INFO - Number of new commits: $num_commits"
#if ($num_commits -le 0) {
#  echo "INFO - Exiting script, there are no new commits for the update.`nSVN branch: *$branch* `nTeamCity build: *$build_number*"
#  SendFailureSlackMessage "Actually, there are no new commits for the update.`nSVN branch: *$branch* `nTeamCity build: *$build_number*"
#  exit 1
#}

# get commit full log for commit range found
#$revision_commit_log = svn log -r "$log_range" https://svn.ankicore.com/svn/victor-wwise/$branch
#$revision_commit_log = [string]::join("`n", $revision_commit_log)
#$revision_commit_log = $revision_commit_log -replace "------------------------------------------------------------------------",""
#$revision_commit_log = $revision_commit_log -replace '"',"'"

# TODO: Look in Git rather than SVN for the commit log (VIC-14176)
$revision_commit_log = "unknown"

# only include commit numbers that exist in branch, the range could contain
# other branch revisions
#$commit_number = $wwise_last_rev
#$commit_range = New-Object System.Collections.ArrayList
#for ($i=0; $i -lt ($num_commits -as [int]); $i++) {
#  $found_commit = $revision_commit_log -match "r$commit_number\s+\|"
#  if ($found_commit) {
#    $commit_range.Add($commit_number)
#    $commit_number--
#  }
#}

# set commit log for new audio assets
#$commit_range.reverse()
#$commit_range = $commit_range -join ','
#if ($branch -eq "trunk") {
#  $revision_commit_log = "Victor Wwise Project Commit $commit_range`n$revision_commit_log"
#} Else {
#  $revision_commit_log = "Victor Wwise Project ($branch) Commit $commit_range`n$revision_commit_log"
#}
echo "INFO - Revision commit log for $branch`: $revision_commit_log"

# checkout audio assets and update file to new revision
$audio_asset_path = "assets"
if (Test-Path $asset_svn_path) {
  svn cleanup $asset_svn_path
  $lastExitCode
  if ($lastExitCode -ne 0) {
    echo "svn cleanup failed. error code $lastExitCode"
    exit $lastExitCode
  }
}

svn co https://svn.ankicore.com/svn/victor-audio-assets/trunk $asset_svn_path --username $env:SVN_USERNAME --password $env:SVN_PASSWORD

# Assure there are no stale assets
Remove-Item -recurse $asset_svn_path\* -exclude .svn
mv $audio_asset_path\* $asset_svn_path

# Add & remove assets then commit
svn status $asset_svn_path

# Force add all new files
svn add --force $asset_svn_path

# Delete missing files
cd $asset_svn_path      # PushD $asset_svn_path
svn status | ? { $_ -match '^!\s+(.*)' } | % { svn rm $Matches[1] }
cd ..                   # PopD

# Commit changes
svn commit $asset_svn_path --message "$revision_commit_log"

# get new revision of assets for Slack message
svn update $asset_svn_path
$soundbanks_revision = svn info $asset_svn_path | select-string -pattern 'Revision:' -casesensitive
$soundbanks_revision = $soundbanks_revision -replace "Revision: ", ""

# send Slack message text that includes new asset revision and commit log
SendSuccessSlackMessage "New revision of Victor sound bank assets have been checked in.`nSVN revision: *$soundbanks_revision* `nSVN branch: *$branch* `nTeamCity build: *$build_number*" $revision_commit_log

exit 0
