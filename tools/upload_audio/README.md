# Upload Audio to Dropbox

### Install:

**Environment**: Python 3, Flask

- Run `tools/setup.py` to install all required libs.

### Setup environment:

To setup the website for upload audio into Dropbox and put information into DynamoDB, we need:

- Dropbox: create a DROPBOX TOKEN as the following guide: https://github.com/dropbox/dropbox-sdk-python and add it in `settings.py`
- DynamoDB: setup the config and credential as the following: https://docs.aws.amazon.com/amazondynamodb/latest/developerguide/SettingUp.DynamoWebService.html

### Running:

- Run `python3 home.py` at the main directory.
- Open http://\<ip address\>:8012
- Click **Upload** if you want to upload audio.
![Imgur Image](https://i.imgur.com/fvLHsuY.png)
- Click **Query** if you want to query the uploaded audio file on DynamoDB.
![Imgur Image](https://i.imgur.com/O2diEQU.png)
