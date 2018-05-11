#!/usr/bin/env python3
from flask import Flask, flash, redirect, render_template, request, session, abort
from werkzeug import secure_filename
import os, json, shutil
from DropboxFileUploader import DropboxFileUploader
from DynamoDB import DynamoDB
import settings

app = Flask(__name__)
app.config.from_object(settings)

SRCDIR = os.path.dirname(os.path.abspath(__file__))
DATADIR = os.path.join(SRCDIR, 'upload')

@app.route("/")
def home():
    return render_template('home.html')

@app.route("/query", methods=['GET', 'POST'])
def query():
    if request.method == 'POST':
        name_obj = request.form['name']
        age = request.form['age']
        distance = request.form['distance']
        date = request.form['date']
        json_data = {}
        if name_obj != "":
            json_data['name'] = name_obj
        if age != "":
            json_data['age'] = age
        if distance != "":
            json_data['distance'] = distance
        if date != "":
            json_data['date'] = date
        if not json_data:
            flash('You should fill at least one field.')
        else:
            dynamoDB = DynamoDB(json_data)
            dataList = dynamoDB.filterValues()
            return render_template('data_table.html', list = dataList)
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
            dropboxUploader = DropboxFileUploader(file_full, dropbox_path)
            dropboxUploader.uploadFile()
            
            ######################

            json_data = {}
            json_data['name'] = name_obj
            json_data['age'] = age
            json_data['distance'] = distance
            json_data['dropbox_path'] = "{}{}".format(dropbox_path, filename)
            json_data['date'] = date
            json_data['gender'] = gender
            
            dynamoDB = DynamoDB(json_data)
            dynamoDB.putItem()
            # Delete temp file
            for the_file in os.listdir(DATADIR):
                file_path = os.path.join(folder, the_file)
                try:
                    if os.path.isfile(file_path):
                        os.unlink(file_path)
                    elif os.path.isdir(file_path):
                        shutil.rmtree(file_path)
                except Exception as e:
                    print(e)
            return render_template('success_message.html', message="File uploaded successfully")
    return render_template('upload.html')
 
if __name__ == "__main__":
    app.run(host='0.0.0.0', port=8012)
