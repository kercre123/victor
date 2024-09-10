$branch = $env:SVN_BRANCH
$revision = $env:SVN_REVISION
$build_number = $env:BUILD_NUMBER
$slack_channel = $env:SLACK_CHANNEL
echo "INFO - env.SVN_BRANCH`: $branch"
echo "INFO - env.SVN_REVISION`: $revision"
echo "INFO - env.BUILD_NUMBER`: $build_number"
echo "INFO - env.SLACK_CHANNEL`: $slack_channel"


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

# Check exit code and maybe send a failure message to slack
function CheckExitCode([Int] $exit_code, [String] $error_message) {
    if ($exit_code -ne 0) {
      echo "INFO - Exiting script. $error_message Build failed. exitCode $exit_code"
      SendFailureSlackMessage "$error_message Build failed. exitCode $exit_code"
      exit $exit_code
    }
}

if (Test-Path $branch) {
    svn cleanup
    CheckExitCode $lastExitCode "svn cleanup failed"
}

echo "INFO - svn co https://svn.ankicore.com/svn/victor-wwise/$branch -r $revision"
svn co https://svn.ankicore.com/svn/victor-wwise/$branch -r $revision $branch --username $env:SVN_USERNAME --password $env:SVN_PASSWORD
CheckExitCode $lastExitCode "svn co failed"
