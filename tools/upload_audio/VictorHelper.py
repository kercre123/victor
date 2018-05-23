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
        result = subprocess.run(command, stdout=subprocess.PIPE, check=True)
        print(result.stdout.decode("utf-8"))
        return result.returncode # return 0 if successfully

    @classmethod
    def pull_data_to_machine(self, ip, output_dir = "./"):
        try:
            return_code = self.pull_data(ip, self.MICDATA_SRC, output_dir)
            return return_code
        except (subprocess.CalledProcessError, subprocess.SubprocessError):
            return "1"


# if __name__ == "__main__":
#     a = VictorHelper.pull_data_to_machine("192.168.2.63","/Users/manh.tran/Desktop/temp")
#     print("a la {}".format(a))
