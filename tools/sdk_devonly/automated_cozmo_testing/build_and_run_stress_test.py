import requests
import os.path
import argparse
import sys
import glob
import subprocess
import re
import json
import zipfile
import devices_util
import platform

try:
  TEAMCITY_USERNAME = os.environ['TEAMCITY_USERNAME']
except KeyError:
  print("Please set the environment variable TEAMCITY_USERNAME")
  sys.exit(1)

try:
  TEAMCITY_PASSWORD = os.environ['TEAMCITY_PASSWORD']
except KeyError:
  print("Please set the environment variable TEAMCITY_PASSWORD")
  sys.exit(1)

args = sys.argv[1:]
parser = argparse.ArgumentParser()
  
parser.add_argument('-p', '--build_folder_path',
                      action='store',
                      required=False,
                      default='/tmp/cozmo_build',
                      help="Where to store build files")

parser.add_argument('-t', '--build_type',
                      action='store',
                      required=False,
                      default='CozmoOne_MasterBuild',
                      help="Teamcity build type to download from")

parser.add_argument('-n', '--build_number',
                      action='store',
                      required=False,
                      default='lastest successful',
                      help="Download specific build number")

parser.add_argument('-B', action='store_true', help="Set Install build flag to true")

parser.add_argument('-S', action='store_true', help="Set Install SDK version flag to true")

parser.add_argument('-R', action='store_true', help="Set Run stress test flag to true")

options = parser.parse_args(args)

AUTH = (TEAMCITY_USERNAME, TEAMCITY_PASSWORD)
HEADERS = {'Accept': 'application/json'}
BUILD_INFO_REST_API_URL = "https://build.ankicore.com/httpAuth/app/rest/builds/"
BUILD_DOWNLOAD_REST_API_URL = "https://build.ankicore.com/repository/download/"
INCLUDED_FILE = [".ipa", ".apk", "sdk_installer"]
EXCLUDED_FILE = ["Cozmo_App_Store", "symbols.zip"]
REGEX_IOS_DEVICES = '.*[iPhone|iPad|Anki].*\[([A-Za-z0-9]*)\]'
REGEX_ANDROID_DEVICES = '^(.*)\s.*device$'
COZMO_PACKAGE_NAME = "com.anki.cozmo"
CURRENT_PATH = os.path.dirname(os.path.realpath(__file__))
STRESS_TEST_SCRIPT = os.path.join(CURRENT_PATH, "SDK_by_Serial.py")

def download_cozmo_build(build_path, build_number):
    result = True
    if not os.path.exists(build_path):
        os.makedirs(build_path)

    response = requests.get('{}buildType:{},number:{}/artifacts'.format(
                            BUILD_INFO_REST_API_URL, options.build_type, build_number), auth=AUTH, headers=HEADERS)
    if response:
        print("Getting build number: {}".format(build_number))
        files = response.json()
        for file in files['file']:
            if is_filename_contains_text(INCLUDED_FILE, file['name']):
                if not is_filename_contains_text(EXCLUDED_FILE, file['name']):
                    print("Checking file: {}...".format(file['name']))
                    if not download_file(build_path, build_number, file['name']):
                        result = False
    else:
        print("Build number: {} is not exists.".format(build_number))
        result = False
    return result

def is_filename_contains_text(text_list, filename):
    res = False
    for item in text_list:
        if item in filename:
            res = True
    return res

def get_lastest_build_number():
    response = requests.get("{}?locator=buildType:{},status:success,count:1".format(
                            BUILD_INFO_REST_API_URL, options.build_type), auth=AUTH, headers=HEADERS)
    build_number = response.json()["build"][0]["number"]
    return build_number


def download_file(build_path, build_number, filename):
    result = True
    download_url = "{}{}/{}/{}".format(BUILD_DOWNLOAD_REST_API_URL, options.build_type, build_number, filename)

    file_path = os.path.join(build_path, filename)
    if os.path.isfile(file_path):
        print("File exists, skipping download...")
    else:
        print("File is downloading, please wait....")
        raw_data = requests.get(download_url, auth=AUTH, allow_redirects=True)
        open(file_path, 'wb').write(raw_data.content)
        if os.path.isfile(file_path):
            print("File is downloaded successfully.")
        else:
            print("Can't download file, please check your network again...")
            result = False
    return result

def install_android_build(build_path, serial_id_list):
    apk_filepath = get_absolute_filepath(build_path, "apk")
    if not apk_filepath:
        return

    for serial_id in serial_id_list:
        print("Installing Cozmo for Android device: {}...".format(devices_util.DevicesInfo.get_device_name(serial_id)))
        package_name = subprocess.run(['adb', '-s', serial_id, 'shell', 'pm', 'list', 'packages', '-f', 
                                        COZMO_PACKAGE_NAME], stdout=subprocess.PIPE)
        if COZMO_PACKAGE_NAME in package_name.stdout.decode("utf-8"):
            uninstall = subprocess.run(['adb', '-s', serial_id, 'uninstall', 
                                        COZMO_PACKAGE_NAME], stdout=subprocess.PIPE)
            print("Uninstall status: {}".format(uninstall.stdout.decode("utf-8")))
        install = subprocess.run(['adb', '-s', serial_id, 'install', apk_filepath], stdout=subprocess.PIPE)
        print("Install status: {}".format(install.stdout.decode("utf-8").splitlines()[-1]))
    print("All Android devices are installed. Check install logs above for more details.")

def install_ios_build(build_path, serial_id_list):
    ipa_filepath = get_absolute_filepath(build_path, "ipa")
    if not ipa_filepath:
        return

    is_ios_deploy_exist = subprocess.run(['npm', 'list', '-g', 'ios-deploy'], stdout=subprocess.PIPE)
    print(is_ios_deploy_exist.stdout.decode("utf-8"))
    if "empty" in is_ios_deploy_exist.stdout.decode("utf-8"):
        print("ios-deploy is not exist, installing...")
        subprocess.run(['npm', 'install', '-g', 'ios-deploy'], stdout=subprocess.PIPE)

    for serial_id in serial_id_list:
        print("Installing Cozmo for iOS device: {}...".format(devices_util.DevicesInfo.get_device_name(serial_id)))
        is_app_installed = subprocess.run(['ios-deploy', '-i', serial_id, '-e', '-1', 
                                            COZMO_PACKAGE_NAME], stdout=subprocess.PIPE)
        if "true" in is_app_installed.stdout.decode("utf-8"):
            uninstall = subprocess.run(['ios-deploy', '-i', serial_id, '-9', '-1', 
                                        COZMO_PACKAGE_NAME], stdout=subprocess.PIPE)
            print("Uninstall status: {}".format(uninstall.stdout.decode("utf-8").splitlines()[-1]))
        install = subprocess.run(['ios-deploy', '-i', serial_id, '-b', ipa_filepath], stdout=subprocess.PIPE)
        print("Install status: {}".format(install.stdout.decode("utf-8").splitlines()[-1]))
    print("All iOS devices are installed. Check install logs above for more details.")

def install_python_sdk_version(build_path):
    path_to_zip_file = get_absolute_filepath(build_path, "zip")
    zip_ref = zipfile.ZipFile(path_to_zip_file, 'r')
    zip_ref.extractall(build_path)
    zip_ref.close()
    execute_file = os.path.join(path_to_zip_file.replace(".zip", ""), "install")
    process = subprocess.Popen(execute_file, stdout=subprocess.PIPE, bufsize=1)
    for line in iter(process.stdout.readline, b''):
        print(line.decode('utf-8'))
    process.stdout.close()
    process.wait()

def get_absolute_filepath(build_path, file_extension):
    file_list = glob.glob("{}/*.{}".format(build_path, file_extension))
    result = ""
    if len(file_list) == 0:
        print("Can't find any package in the given folder: {}".format(build_path))
    else:
        result = file_list[0]
        if len(file_list) > 1:
            print("More than one build were found. First build: {} will be installed.".format(file_list[0]))
    return result

def get_serial_id_list(result, regex):
    lines = result.stdout.decode("utf-8").splitlines()
    serial_list = []
    for line in lines:
        serialID = re.search(regex, line)
        if serialID:
            serial_list.append(serialID.group(1))
    return serial_list

def run_test_for_device(device_platform, serial_id_list):
    current_platform = platform.system().lower()
    if current_platform == "windows":
        for serial_id in serial_id_list:
            print("Run Stress test for Cozmo: {}...".format(devices_util.DevicesInfo.get_device_name(serial_id)))
            os.system("start cmd /k python3 {} -{} -S {}".format(STRESS_TEST_SCRIPT, device_platform, serial_id))
    elif current_platform == "darwin":
        for serial_id in serial_id_list:
            print("Run Stress test for Cozmo: {}...".format(devices_util.DevicesInfo.get_device_name(serial_id)))
            command = "tell application \"Terminal\" to do script \"python3 {} -{} -S {}\"".format(
                        STRESS_TEST_SCRIPT, device_platform, serial_id)
            subprocess.call(['osascript', '-e', command])

if __name__ == "__main__":
    build_number = options.build_number
    folder_path = options.build_folder_path

    if build_number == "lastest successful":
        build_number = get_lastest_build_number()
    build_path = os.path.join(folder_path, build_number)

    android_p = subprocess.run(['adb', 'devices'], stdout=subprocess.PIPE)
    android_serial_id_list = get_serial_id_list(android_p, REGEX_ANDROID_DEVICES)
    ios_p = subprocess.run(['instruments', '-s', 'devices'], stdout=subprocess.PIPE)
    ios_serial_id_list = get_serial_id_list(ios_p, REGEX_IOS_DEVICES)

    #If all tags are ignored, run all process
    if     options.B == options.S == options.R == False:
        options.B = options.S = options.R = True

    if options.B == True:
        if download_cozmo_build(build_path, build_number):
            if android_serial_id_list:
                install_android_build(build_path, android_serial_id_list)
            else:
                print("No Android devices found.")

            if ios_serial_id_list:
                install_ios_build(build_path, ios_serial_id_list)
            else:
                print("No iOS devices found.")

    if options.S == True:
        if download_cozmo_build(build_path, build_number):
            install_python_sdk_version(build_path)

    if options.R == True:
        input("Please make sure all Cozmo[s] are opened in SDK mode. "\
              "Press Enter to continue when they're ready...")
        if android_serial_id_list:
            run_test_for_device("A", android_serial_id_list)
        else:
            print("No Android devices found.")

        if ios_serial_id_list:
            run_test_for_device("I", ios_serial_id_list)
        else:
            print("No iOS devices found.")
