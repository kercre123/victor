#!/usr/bin/env python3
import subprocess

# For pulling micdata on Victor to local machine use :
#           VictorHelper.pull_data_to_machine(<robot_ip>, output_dir)

class VictorHelper(object) :

    VECTOR_USER          = "root"
    MICDATA_SRC          = "/data/data/com.anki.victor/cache/micdata"
    SCP_COMMAND          = "scp"

    @classmethod
    def pull_data_command(self, vector_ip, src_path, dest_path):
        src_command = "{}@{}:{}".format(self.VECTOR_USER, vector_ip, src_path)
        return [ self.SCP_COMMAND, "-r", src_command, dest_path ]

    @classmethod
    def pull_data(self, vector_ip, src_path, dest_path):
        command = self.pull_data_command(vector_ip, src_path, dest_path)
        result = subprocess.run(command, stdout=subprocess.PIPE)
        print(result.stdout.decode("utf-8"))

    @classmethod
    def pull_data_to_machine(self, ip, output_dir = "./"):
        self.pull_data(ip, self.MICDATA_SRC, output_dir)

# if __name__ == "__main__":
#     VictorHelper.pull_data_to_machine("192.168.42.22","/Users/son.nguyen/Desktop/son.nguyen/temp/kaka2")