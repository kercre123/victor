#!/usr/bin/env python3
from flask import Flask, flash, redirect, render_template, request, session, abort
from werkzeug import secure_filename
import os, json, shutil
from DropboxFileUploader import DropboxFileUploader
from DynamoDB import DynamoDB
from VictorHelper import VictorHelper
import settings

app = Flask(__name__)
app.config.from_object(settings)

upload_folder = 'upload'
SRCDIR = os.path.dirname(os.path.abspath(__file__))
DATADIR = os.path.join(SRCDIR, upload_folder)

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
            data_list = dynamo_db.filterValues()
            return render_template('data_table.html', list = data_list)
    return render_template('query.html')

@app.route("/upload", methods=['GET', 'POST'])
def upload():
    if request.method == 'POST':
        name_obj = request.form['name']
        age = request.form['age']
        distance = request.form['distance']
        dropbox_path = request.form['dropbox_path']
        date = request.form['date']
        gender = request.form['radio']
        file_obj = request.files['file']

        if file_obj:
            filename = secure_filename(file_obj.filename)
            file_full = os.path.join(DATADIR, filename)
            file_obj.save(file_full)

            if not dropbox_path.startswith("/"):
                dropbox_path = "/{}".format(dropbox_path)
            if not dropbox_path.endswith("/"):
                dropbox_path = "{}/".format(dropbox_path)

            path_db = "{}{}".format(dropbox_path, filename)
            dropbox_uploader = DropboxFileUploader(file_full, dropbox_path)
            dropbox_uploader.uploadFile()
            
            ######################

            json_data = {}
            json_data[column_name] = name_obj
            json_data[column_age] = age
            json_data[column_distance] = distance
            json_data[column_dropbox_path] = "{}{}".format(dropbox_path, filename)
            json_data[column_date] = date
            json_data[column_gender] = gender
            
            dynamo_db = DynamoDB(json_data)
            dynamo_db.putItem()
            # Delete temp file
            for the_file in os.listdir(DATADIR):
                file_path = os.path.join(upload_folder, the_file)
                try:
                    if os.path.isfile(file_path):
                        os.unlink(file_path)
                    elif os.path.isdir(file_path):
                        shutil.rmtree(file_path)
                except Exception as e:
                    print(e)
            return render_template('success_message.html', message="File uploaded successfully")
    return render_template('upload.html')

###############################
@app.route("/upload-from-victor", methods=['GET', 'POST'])
def upload_from_victor():
    ERROR_EMPTY_MESSAGE = 'You should fill Victor\'s IP field.'
    ERROR_NOT_FOUND_MESSAGE = 'Cannot find any Victor that match the IP address. Please try again.'
    if request.method == 'POST':
        victor_ip = request.form['ip']

        if victor_ip != "":
            VictorHelper.pull_data_to_machine(victor_ip, )
        else:
            flash(ERROR_EMPTY_MESSAGE)

        if file_obj:
            filename = secure_filename(file_obj.filename)
            file_full = os.path.join(DATADIR, filename)
            file_obj.save(file_full)

            if not dropbox_path.startswith("/"):
                dropbox_path = "/{}".format(dropbox_path)
            if not dropbox_path.endswith("/"):
                dropbox_path = "{}/".format(dropbox_path)

            path_db = "{}{}".format(dropbox_path, filename)
            dropbox_uploader = DropboxFileUploader(file_full, dropbox_path)
            dropbox_uploader.uploadFile()
            
            ######################

            json_data = {}
            json_data[column_name] = name_obj
            json_data[column_age] = age
            json_data[column_distance] = distance
            json_data[column_dropbox_path] = "{}{}".format(dropbox_path, filename)
            json_data[column_date] = date
            json_data[column_gender] = gender

            dynamo_db = DynamoDB(json_data)
            dynamo_db.putItem()
            # Delete temp file
            for the_file in os.listdir(DATADIR):
                file_path = os.path.join(upload_folder, the_file)
                try:
                    if os.path.isfile(file_path):
                        os.unlink(file_path)
                    elif os.path.isdir(file_path):
                        shutil.rmtree(file_path)
                except Exception as e:
                    print(e)
            return render_template('success_message.html', message="File uploaded successfully")
    return render_template('find_victor_audio_files.html')
################################################################
if __name__ == "__main__":
    app.run(host='0.0.0.0', port=8012)
