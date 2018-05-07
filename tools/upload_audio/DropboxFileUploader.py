#!python3.5

# Prerequisites :
# 1.SetUp dropbox sdk to be able to use Dropbox Api's
# $ sudo pip install dropbox
# By default python dropbox sdk is based upon the python 3.5
#
# 2. Create an App on dropbox console (https://www.dropbox.com/developers/apps) which will be used and validated to do
# the file upload and restore using dropbox api. Mostly you need an access token to connect to Dropbox before actual file/folder operations.
#
# 3. Once done with code, run the script by following command
# $ python SFileUploader.py // if python3.5 is default


import sys, os
import dropbox

from dropbox.files import WriteMode
from dropbox.exceptions import ApiError, AuthError
import settings

class DropboxFileUploader():

    TOKEN = settings.TOKEN

    LOCALFILE = ""
    BACKUPPATH = "" # Keep the forward slash before destination filename
    FOLDERPATH = ""

    def __init__(self, local_file, folder_path):
        self.LOCALFILE = local_file
        self.FOLDERPATH = folder_path

    # Uploads contents of LOCALFILE to Dropbox
    def backup(self, dbx):
        self.BACKUPPATH = "{}{}".format(self.FOLDERPATH, os.path.basename(self.LOCALFILE))
        with open(self.LOCALFILE, 'rb') as f:
            # We use WriteMode=overwrite to make sure that the settings in the file
            # are changed on upload
            print("Uploading " + self.LOCALFILE + " to Dropbox as " + self.BACKUPPATH + "...")
            try:
                dbx.files_upload(f.read(), self.BACKUPPATH, mode=WriteMode('overwrite'))
            except ApiError as err:
                # This checks for the specific error where a user doesn't have enough Dropbox space quota to upload this file
                if (err.error.is_path() and
                        err.error.get_path().error.is_insufficient_space()):
                    sys.exit("ERROR: Cannot back up; insufficient space.")
                elif err.user_message_text:
                    print(err.user_message_text)
                    sys.exit()
                else:
                    print(err)
                    sys.exit()


    # Adding few functions to check file details
    def checkFileDetails(self, dbx):
        print("Checking file details")

        for entry in dbx.files_list_folder('').entries:
            print("File list is : ")
            print(entry.name)

    def uploadFile(self):
        # Check for an access token
        if (len(self.TOKEN) == 0):
            sys.exit("ERROR: Looks like you didn't add your access token. Open up backup-and-restore-example.py in a text editor and paste in your token in line 14.")

        # Create an instance of a Dropbox class, which can make requests to the API.
        print("Creating a Dropbox object...")
        dbx = dropbox.Dropbox(self.TOKEN)

        # Check that the access token is valid
        try:
            dbx.users_get_current_account()
        except AuthError as err:
            sys.exit(
                "ERROR: Invalid access token; try re-generating an access token from the app console on the web.")

        self.checkFileDetails(dbx)

        print("Creating backup...")
        # Create a backup of the current settings file
        self.backup(dbx)

        print("Done!")
