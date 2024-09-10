#!/usr/bin/env python3

"""This script generates protobuf python files.

Includes the following:
- Removes old protobuf files.
- Copies proto from the "victor-proto" submodule to "sdk/anki_vector/messages" (default).
- Modifies import path of proto files.
- Removes app-only code (optional).
- Uses protoc to generate python code.
- Generates protobuf object documentation for sdk docs.
"""

import argparse
import glob
import os
from pathlib import Path
import re
from shutil import copy2
import shlex
import subprocess
import sys


def remove_old_files(output):
    for proto_file in glob.glob(str(output) + "/*.proto"):
        os.remove(proto_file)
    for proto_out_file in glob.glob(str(output) + "/*_pb2*.py"):
        os.remove(proto_out_file)


def copy_proto_files(proto_dir, output):
    for proto_file in glob.glob(str(proto_dir) + "/*.proto"):
        copy2(proto_file, output)


def edit_proto_files(output, skip_edit, skip_path):
    print("> Editing proto file imports...", end="")
    for proto_file in glob.glob(str(output) + "/*.proto"):
        with open(proto_file, "r") as sources:
            lines = sources.readlines()
        with open(proto_file, "w") as sources:
            for line in lines:
                if not skip_edit:
                    if "*** App only proto below ***" in line:
                        if "external_interface" in proto_file:
                            sources.write("}")
                        break
                    if "// App only" in line:
                        continue
                if not skip_path:
                    line = re.sub(r'import "([^\/]*).proto"', 'import "anki_vector/messaging/\\1.proto"', line)
                sources.write(line)
    print(" DONE")


def download_deps(working_dir):
    deps_dir = working_dir / "deps"
    os.makedirs(str(deps_dir.resolve()), exist_ok=True)

    grpc_gateway_dir = deps_dir / "temp-grpc-gateway"
    print("> Cloning deps/temp-grpc-gateway")
    if not grpc_gateway_dir.exists():
        git_command = shlex.split(f"git clone https://github.com/grpc-ecosystem/grpc-gateway.git {str(grpc_gateway_dir)}")
        git_process = subprocess.Popen(git_command, cwd=working_dir)
        git_process.wait()
    else:
        print("deps/temp-grpc-gateway already exists")

    print("> Finding go installation")
    which_go = subprocess.Popen(shlex.split("which go"), cwd=working_dir)
    which_go.wait()
    if which_go.returncode:
        sys.exit(f"ERROR: Did not find existing go binary.\n\n\tPlease install go with 'brew install go'\n")

    go_root = deps_dir / "go"
    print("> Creating deps/go")
    if not go_root.exists():
        modified_env = os.environ.copy()
        modified_env["GOPATH"] = str(go_root.resolve())
        os.makedirs(str(go_root.resolve()), exist_ok=True)
        git_command = shlex.split(f"go get -u github.com/pseudomuto/protoc-gen-doc/cmd/protoc-gen-doc")
        git_process = subprocess.Popen(git_command, cwd=working_dir, env=modified_env)
        git_process.wait()
    else:
        print("deps/go already exists")
    return grpc_gateway_dir, go_root


def generate_pb2_files(working_dir, output, skip_path):
    grpc_gateway_dir, go_root = download_deps(working_dir)

    print("> Generating proto files...", end="")
    proto_files = sorted(glob.glob(str(output) + "/*.proto"))
    if not skip_path:
        sdk_root = (output / ".." / "..").resolve()
    else:
        sdk_root = output.resolve()
    modified_env = os.environ.copy()
    modified_env["PATH"] = str((go_root / "bin").resolve()) + ":" + modified_env["PATH"]
    protoc_command = shlex.split("python3 -m grpc_tools.protoc "
                                 f"-I {sdk_root} "
                                 f"-I {str(grpc_gateway_dir.resolve())}/third_party/googleapis "
                                 f"--python_out={sdk_root} "
                                 f"--grpc_python_out={sdk_root} "
                                 f"{'--doc_out=sdk/docs/source ' if not skip_path else ''}"
                                 f"{'--doc_opt=sdk/docs/source/template.tmpl,proto.html:alexa*,settings* ' if not skip_path else ''}"
                                 f"{' '.join(proto_files)}")
    protoc_process = subprocess.Popen(protoc_command, cwd=working_dir, env=modified_env)
    protoc_process.wait()
    if protoc_process.returncode:
        sys.exit(" ERROR")
    print(" DONE")


def update_python_proto(working_dir, proto_dir, output, skip_edit, skip_path):
    remove_old_files(output)
    copy_proto_files(proto_dir, output)
    edit_proto_files(output, skip_edit, skip_path)
    generate_pb2_files(working_dir, output, skip_path)


def main():
    working_dir = Path(__file__).resolve().parent

    parser = argparse.ArgumentParser(
        description=(
            "Update the proto files from the a given commit of the victor-proto repo."
            "To make changes to the proto files, you need to modify them in the victor repo and push to the victor-proto subtree."
        ))
    parser.add_argument("-d", "--dir", help="Custom dir from which to grab the proto files.", default=str(working_dir / "victor-proto"))
    parser.add_argument("-o", "--output", help="Custom output location.", default=str(working_dir / "sdk/anki_vector/messaging"))
    parser.add_argument("-s", "--skip_edit", action="store_true", help="Do not remove the app-only comments out of proto files after copying.")
    parser.add_argument("-p", "--skip_path", action="store_true", help=("Do not change import path of proto files after copying. "
                                                                       "This can be used to provide the python output outside of the SDK. "
                                                                       "Good for directly testing protobuf."))
    args = parser.parse_args()

    proto_dir = Path(args.dir).resolve()
    public_proto_dir = proto_dir / "public"

    output_dir = Path(args.output).resolve()

    update_python_proto(working_dir, public_proto_dir, output_dir, args.skip_edit, args.skip_path)


if __name__ == "__main__":
    main()
