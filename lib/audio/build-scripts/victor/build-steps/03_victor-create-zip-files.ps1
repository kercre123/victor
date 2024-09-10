
$workspace_dir = $env:PROJECT_WORKSPACE
$git_repo_name = $env:GIT_REPO_NAME
$slack_channel = $env:SLACK_CHANNEL

$env:path = "$env:Path;c:\tools\python2"

$_REPO_LOCATION = [string](Get-Location)
$_FIX_M4A_TIMESTAMPS_SCRIPT_PATH = "$_REPO_LOCATION\build-scripts\fix_m4a_timestamps_in_wwise_file.py"
$_BUNDLE_METADATA_PYTHON_SCRIPT_PATH = "$_REPO_LOCATION\build-scripts\bundle_metadata_products.py"
$_BUNDLE_ASSETS_PYTHON_SCRIPT_PATH = "$_REPO_LOCATION\build-scripts\bundle_soundbank_products.py"
$_ORGANIZE_ASSETS_PYTHON_SCRIPT_PATH = "$_REPO_LOCATION\build-scripts\organize_soundbank_products.py"
$_CONVERT_CSV_TO_JSON_SCRIPT_PATH = "$_REPO_LOCATION\build-scripts\convert_csv_to_json.py"

$_PROJ_DIR = "$_REPO_LOCATION\${workspace_dir}\${git_repo_name}\VictorAudio"
$_METADATA_DIR = "$_REPO_LOCATION\${workspace_dir}\${git_repo_name}\metadata"
$_GSB_DIR = "$_PROJ_DIR\GeneratedSoundBanks"

# Victor Robot App Platforms
$_SOUNDS_DIR_VICTOR_LINUX =     "$_GSB_DIR\Victor_Linux"

# Development Victor Platform
$_SOUNDS_DIR_DEV_MAC =          "$_GSB_DIR\Dev_Mac"

# Asset Dirs
$_ASSETS_DIR = "assets"
$_TMP_OUTPUT_DIR_VICTOR_LINUX =     "$_ASSETS_DIR\victor_linux"
$_TMP_OUTPUT_DIR_DEV_MAC =          "$_ASSETS_DIR\dev_mac"

# Metadata
$_APP_METADATA = "$_ASSETS_DIR\metadata"

$_SKU_DIR = "victor_robot"
$_VICTOR_BANK_LIST_PATH = "$_METADATA_DIR\victor-banks-list.json"
$_VICTOR_LINUX = "$_ASSETS_DIR\$_SKU_DIR\victor_linux"

# NOTE: We are currently only using Mac platform for Webots simulator
$_VICTOR_DEV_MAC = "$_ASSETS_DIR\$_SKU_DIR\dev_mac"

if (Test-Path $_ASSETS_DIR) {
    Remove-Item $_ASSETS_DIR -recurse
}

# Make Dirs
mkdir $_TMP_OUTPUT_DIR_VICTOR_LINUX
mkdir $_TMP_OUTPUT_DIR_DEV_MAC

mkdir $_APP_METADATA
mkdir $_VICTOR_LINUX
mkdir $_VICTOR_DEV_MAC

echo "INFO - env:GIT_REPO_NAME`: $git_repo_name"
echo "INFO - env.SLACK_CHANNEL`: $slack_channel"
echo $_REPO_LOCATION
echo $_FIX_M4A_TIMESTAMPS_SCRIPT_PATH
echo $_BUNDLE_METADATA_PYTHON_SCRIPT_PATH
echo $_BUNDLE_ASSETS_PYTHON_SCRIPT_PATH
echo $_ORGANIZE_ASSETS_PYTHON_SCRIPT_PATH
echo $_CONVERT_CSV_TO_JSON_SCRIPT_PATH
echo $_PROJ_DIR
echo $_GSB_DIR
# Platform Paths
echo $_SOUNDS_DIR_VICTOR_LINUX
echo $_SOUNDS_DIR_DEV_MAC
echo $_ASSETS_DIR
echo $_TMP_OUTPUT_DIR_VICTOR_LINUX
echo $_TMP_OUTPUT_DIR_DEV_MAC
echo $_APP_METADATA
echo $_VICTOR_BANK_LIST_PATH
echo $_VICTOR_LINUX
echo $_VICTOR_DEV_MAC

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
  Invoke-RestmethVR -MethVR POST -BVRy ($payload | ConvertTo-Json -Depth 4) -Uri $env:SLACK_TOKEN_URL
}

# send a failure message to slack channel
function CheckExitCode([Int] $exit_code, [String] $error_message) {
    if ($exit_code -ne 0) {
      echo "INFO - Exiting script, there was a problem creating sound bank .zip files , build failed. exitCode $exit_code : $error_message"
      SendFailureSlackMessage "There was a problem creating sound bank .zip files, build failed. exitCode $exit_code : $error_message"
      exit $exit_code
    }
}

# Copy metadata files .txt .json .xml
python $_BUNDLE_METADATA_PYTHON_SCRIPT_PATH $_GSB_DIR $_APP_METADATA
CheckExitCode $lastExitCode

# Fix M4A time stamps so we can compare hash of current assets
# Mac
python $_FIX_M4A_TIMESTAMPS_SCRIPT_PATH $_TMP_OUTPUT_DIR_DEV_MAC
CheckExitCode $lastExitCode "There was a problem fixing the timestamps in the .bnk/.wem files in $_TMP_OUTPUT_DIR_DEV_MAC."

# Bundle Victor assets
python $_BUNDLE_ASSETS_PYTHON_SCRIPT_PATH $_SOUNDS_DIR_VICTOR_LINUX $_TMP_OUTPUT_DIR_VICTOR_LINUX
CheckExitCode $lastExitCode

# Bundle DEV Victor assets
python $_BUNDLE_ASSETS_PYTHON_SCRIPT_PATH $_SOUNDS_DIR_DEV_MAC $_TMP_OUTPUT_DIR_DEV_MAC
CheckExitCode $lastExitCode

# Organize sound bank assets
# Victor - Robot
python $_ORGANIZE_ASSETS_PYTHON_SCRIPT_PATH $_TMP_OUTPUT_DIR_VICTOR_LINUX $_VICTOR_LINUX $_VICTOR_BANK_LIST_PATH --unzip-bundles
CheckExitCode $lastExitCode

# Victor - Dev-Webots
python $_ORGANIZE_ASSETS_PYTHON_SCRIPT_PATH $_TMP_OUTPUT_DIR_DEV_MAC $_VICTOR_DEV_MAC $_VICTOR_BANK_LIST_PATH --unzip-bundles
CheckExitCode $lastExitCode

# Copy audio behavior metadata to Victor project asset directories
$_AUDIO_BEHAVIOR_FILE_NAME = "audioBehaviorSceneEvents"
$_AUDIO_BEHAVIOR_CSV_FILE_PATH = "$_METADATA_DIR\$_AUDIO_BEHAVIOR_FILE_NAME.csv"
python $_CONVERT_CSV_TO_JSON_SCRIPT_PATH $_AUDIO_BEHAVIOR_CSV_FILE_PATH "$_VICTOR_LINUX\$_AUDIO_BEHAVIOR_FILE_NAME.json" -d
CheckExitCode $lastExitCode

python $_CONVERT_CSV_TO_JSON_SCRIPT_PATH $_AUDIO_BEHAVIOR_CSV_FILE_PATH "$_VICTOR_DEV_MAC\$_AUDIO_BEHAVIOR_FILE_NAME.json" -d
CheckExitCode $lastExitCode

# Delete temp copy
if (Test-Path $_ASSETS_DIR) {
    Remove-Item $_TMP_OUTPUT_DIR_VICTOR_LINUX -recurse
    Remove-Item $_TMP_OUTPUT_DIR_DEV_MAC -recurse
}

exit 0
