#!/usr/bin/env python3
import subprocess
import argparse, os, sys

# For pulling micdata on Victor to local machine use :
#           VictorHelper.pull_data_to_machine(<robot_ip>, output_dir)

class VictorHelper(object) :

    VECTOR_USER          = "root"
    MICDATA_SRC          = "/data/data/com.anki.victor/cache/micdata"
    SCP_COMMAND          = "scp"

    @classmethod
    def parse_arguments(self, args):
        parser = argparse.ArgumentParser()
        parser.add_argument('-ip', '--ip', action="store", required=True, default="", help="Victor's ip")
        options = parser.parse_args(args)
        return options

    @classmethod
    def pull_data_command(self, vector_ip, src_path, dest_path):
        src_command = "{}@{}:{}".format(self.VECTOR_USER, vector_ip, src_path)
        return [self.SCP_COMMAND, "-r", src_command, dest_path]

    @classmethod
    def pull_data(self, vector_ip, src_path, dest_path):
        command = self.pull_data_command(vector_ip, src_path, dest_path)
        result = subprocess.run(command, stdout=subprocess.PIPE, check=True)
        print(result.stdout.decode("utf-8"))
        return result.returncode # return 0 if successfully

    @classmethod
    def pull_data_to_machine(self, ip, output_dir = "./"):
        try:
            return_code = self.pull_data(ip, self.MICDATA_SRC, output_dir)
            return return_code
        except (subprocess.CalledProcessError, subprocess.SubprocessError):
            return 1


if __name__ == "__main__":
    audio_folder = 'victor_audio'
    src_dir = os.path.dirname(os.path.abspath(__file__))
    data_dir = os.path.join(src_dir, audio_folder)
    option = VictorHelper.parse_arguments(sys.argv[1:])
    result = VictorHelper.pull_data_to_machine(option.ip, data_dir)
    if result == 0:
        print("Copied all audio files from victor to Upload folder.")
    else:
        print("Cannot find any Victor that match the IP address. Please try again.")
