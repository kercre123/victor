#!/usr/bin/env python3
from flask import Flask, flash, redirect, render_template, request, session, abort
from werkzeug import secure_filename
import os, json, shutil
from DropboxFileUploader import DropboxFileUploader
from DynamoDB import DynamoDB
from VictorHelper import VictorHelper
import settings
import time
from CommonHelper import CommonHelper

app = Flask(__name__)
app.config.from_object(settings)

SRCDIR = os.path.dirname(os.path.abspath(__file__))
# The name of column in table
column_name = 'name'
column_age = 'age'
column_distance = 'distance'
column_date = 'date'
column_dropbox_path = 'dropbox_path'
column_gender = 'gender'

@app.route("/")
def home():
    return render_template('home.html')

@app.route("/query", methods=['GET', 'POST'])
def query():
    ERROR_MESSAGE = 'You should fill at least one field.'
    if request.method == 'POST':
        name_obj = request.form['name']
        age = request.form['age']
        distance = request.form['distance']
        date = request.form['date']
        json_data = {}
        if name_obj != "":
            json_data[column_name] = name_obj
        if age != "":
            json_data[column_age] = age
        if distance != "":
            json_data[column_distance] = distance
        if date != "":
            json_data[column_date] = date
        if not json_data:
            flash(ERROR_MESSAGE)
        else:
            dynamo_db = DynamoDB(json_data)
            data_list = dynamo_db.filter_values()
            return render_template('data_table.html', list = data_list)
    return render_template('query.html')

@app.route("/upload-from-victor", methods=['GET', 'POST'])
def upload_from_victor():
    ERROR_EMPTY_MESSAGE = 'You should fill Victor\'s IP field.'
    ERROR_NOT_FOUND_MESSAGE = 'Cannot find any Victor that match the IP address. Please try again.'
    # setup variable
    time_string = CommonHelper().get_time_string()
    upload_folder = "{}_{}".format("upload", time_string)
    DATADIR = os.path.join(SRCDIR, upload_folder)

    format_datetime = CommonHelper().get_datetime_format()
    dropbox_folder = "/Anki/{}/".format(format_datetime)
    if request.method == 'POST':
        user_name = request.form['name']
        age = request.form['age']
        distance = request.form['distance']
        gender = request.form['radio']
        victor_ip = request.form['vic_ip']
        prefix_name = "{}_{}_{}_{}".format(user_name, age, gender, distance)
        if (victor_ip.strip() != "" and user_name.strip() != ""):
            result = VictorHelper.pull_data_to_machine(victor_ip, DATADIR)
            if result == 1:
                flash(ERROR_NOT_FOUND_MESSAGE)
            else:
                # upload to dynamodb
                json_data = {}
                json_data[column_name] = user_name
                json_data[column_age] = age
                json_data[column_distance] = distance
                json_data[column_date] = format_datetime
                json_data[column_gender] = gender
                
                dynamo_db = DynamoDB(json_data)
                dynamo_db.put_item_into_db()
                CommonHelper().add_prefix_files_in_directory(DATADIR, prefix_name)
                dropbox_uploader = DropboxFileUploader(DATADIR, dropbox_folder)
                dropbox_uploader.upload_folder()
                # remove temp files
                CommonHelper().delete_folder(DATADIR)
                return render_template('success_message.html', message="File uploaded successfully")
        else:
            flash(ERROR_EMPTY_MESSAGE)
    return render_template('find_victor_audio_files.html')
################################################################
if __name__ == "__main__":
    app.run(host='0.0.0.0', port=8012)
