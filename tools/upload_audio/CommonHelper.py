#!/usr/bin/env python3

import os, shutil

class CommonHelper() :

    def delete_folder(self, folder_path, folder_name):
        for the_file in os.listdir(folder_path):
                file_path = os.path.join(folder_name, the_file)
                try:
                    if os.path.isfile(file_path):
                        os.unlink(file_path)
                    elif os.path.isdir(file_path):
                        shutil.rmtree(file_path)
                except Exception as e:
                    print(e)
