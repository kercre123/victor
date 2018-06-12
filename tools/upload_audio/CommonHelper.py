#!/usr/bin/env python3

import os, shutil, time

class CommonHelper() :

    def create_folder(self, folder_path):
        if not os.path.exists(folder_path):
            os.makedirs(folder_path)

    def delete_folder(self, folder_path):
        shutil.rmtree(folder_path, ignore_errors=True)

    def get_time_string(self):
        return time.strftime("%Y%m%d%H%M%S")

    def get_datetime_format(self):
        return time.strftime("%Y_%m_%d")

    def add_prefix_files_in_directory(self, directory, expected_prefix_name):
        for root, dirs, files in os.walk(directory):
            for filename in files:
                os.rename(os.path.join(root, filename),
                          os.path.join(root, "{}_{}".format(expected_prefix_name, filename)))
