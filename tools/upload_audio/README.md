# Upload Audio to Dropbox

### How to use:

**Environment**: Python 3, Flask

To setup the website for upload audio into Dropbox and put information into DynamoDB, we need:
- Flask: create a random SECRECT_KEY as example in `settings.py`
- Dropbox: create a TOKEN as following the guide: https://github.com/dropbox/dropbox-sdk-python and add it in `settings.py`
- DynamoDB: setup this as the following: https://docs.aws.amazon.com/amazondynamodb/latest/developerguide/SettingUp.DynamoWebService.html

**Running**:
- Run `python3 home.py` at the main directory.
- Open http://localhost:8012
- Click **Upload** if you want to upload audio
- Click **Query** if you want to query the uploaded audio file on DynamoDB
