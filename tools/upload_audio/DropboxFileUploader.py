import sys, os
import dropbox

from dropbox.files import WriteMode
from dropbox.exceptions import ApiError, AuthError
import settings

class DropboxFileUploader():

    token = settings.TOKEN

    local_file_dbx = ""
    backup_path = "" # Keep the forward slash before destination filename
    folder_path_dbx = ""

    def __init__(self, local_file, folder_path):
        self.local_file_dbx = local_file
        self.folder_path_dbx = folder_path

    def dropbox_instance(self):
        # Check for an access token
        if (len(self.token) == 0):
            sys.exit("ERROR: Looks like you didn't add your access token.")

        # Create an instance of a Dropbox class, which can make requests to the API.
        print("Creating a Dropbox object...")
        dbx = dropbox.Dropbox(self.token)

        # Check that the access token is valid
        try:
            dbx.users_get_current_account()
        except AuthError as err:
            sys.exit(
                "ERROR: Invalid access token; try re-generating an access token from the app console on the web.")
        return dbx

    # Uploads contents of localfile to Dropbox
    def upload_to_dropbox(self, dbx):
        self.backup_path = "{}{}".format(self.folder_path_dbx, os.path.basename(self.local_file_dbx))
        with open(self.local_file_dbx, 'rb') as f:
            # We use WriteMode=overwrite to make sure that the settings in the file
            # are changed on upload
            print("Uploading {} to Dropbox as {}...".format(self.local_file_dbx, self.backup_path))
            try:
                dbx.files_upload(f.read(), self.backup_path, mode=WriteMode('overwrite'))
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

    def upload_file(self):
        dbx = self.dropbox_instance()

        print("Creating backup...")
        # Create a backup of the current settings file
        self.upload_to_dropbox(dbx)

        print("Done!")

    def upload_folder(self):
        dbx = self.dropbox_instance()

        # enumerate local files recursively
        for root, dirs, files in os.walk(self.local_file_dbx):
            for filename in files:
                # construct the full local path
                self.local_file_dbx = os.path.join(root, filename)
                self.upload_to_dropbox(dbx)
