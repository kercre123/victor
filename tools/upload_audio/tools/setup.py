#!/usr/bin/env python3
import pip
import os
import pkg_resources
import sys

def install(package):
    pip.main(['install', package])

def get_installed_pip():
    packages = pkg_resources.find_distributions(sys.path[-1])
    package_names = []
    for package in packages:
        package_names.append(package.project_name.lower().replace("-","_"))
    return package_names

if __name__ == '__main__':
    current_path = os.path.dirname(os.path.abspath(__file__))
    lib_file = os.path.join(current_path, "libs.txt")
    package_names = get_installed_pip()
    libs = []
    with open(lib_file, "r") as file:
        for lib in file:
            if not lib.startswith('#') and str(lib).strip() != "":
                if str(lib).strip().lower() not in package_names:
                    print("---------------------------------------")
                    print("Installing {}".format(lib))
                    install(lib)
                    libs.append(lib.strip().lower())

    package_names = get_installed_pip()
    failed_count = 0
    output = ""

    for lib in libs:
        if lib not in package_names:
            failed_count += 1
            output = "{} - {}".format(output, lib)

    if failed_count == 0:
        print("All packages are installed successfully.")
    else:
        print("Packages are installed unsuccessfully:{}".format(output))
