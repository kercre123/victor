import os

# Drop this file in the directory where the logs are stored
LOG_DUMP_DIR = os.path.dirname(os.path.abspath(__file__))
LOG_FILE_PATH = os.path.join(LOG_DUMP_DIR, "log.txt")
CALIB_VALUES_FILE_PATH = os.path.join(LOG_DUMP_DIR, "calib_values.txt")
CHECK_VARS = ["ComputeCalibrationFromSingleImage.D", "ComputeCalibrationFromSingleTarget.CalibValues", "CameraCalibrator.ComputeCalibrationFromSingleTarget.Marker"]
CALIB_TEXT = "Calibration Values - Set %d:\n"

if __name__ == "__main__":
        calib_values = []
        calib_value_count = 1
        # Check the count of the last of calibration values
        if os.path.exists(CALIB_VALUES_FILE_PATH):
                f = open(CALIB_VALUES_FILE_PATH, 'r')
                lines = f.readlines()
                try:
                        calib_value_count += int(lines[-4].split()[-1][:-1])
                        print("Starting calibration set count from: " + str(calib_value_count))
                except:
                        print("CALIB_VALUES_FILE_PATH is empty")
                f.close()
        # Read log file and extract calibration values
        log_file = open(LOG_FILE_PATH, 'r')
        log_lines = log_file.readlines()
        log_file.close()
        for line in log_lines:
                if CHECK_VARS[0] in line:
                        calib_values.append(CALIB_TEXT % calib_value_count)
                        calib_values.append(line.split(":")[-1])
                        calib_value_count += 1
                elif CHECK_VARS[1] in line:
                        calib_values.append(" fx" + line.split("fx")[-1] + "\n")
                elif CHECK_VARS[2] in line:
                        calib_values.append(line.split("top left : ")[-1])
        # Write the extracted calibration values to a text file
        calib_file = open(CALIB_VALUES_FILE_PATH, "a+")
        calib_file.writelines(calib_values)
        calib_file.close()
        