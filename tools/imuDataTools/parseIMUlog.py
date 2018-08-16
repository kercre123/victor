#!/usr/bin/env python3
"""Python utility for parsing IMU data logs.
   The log is triggered by pressing the 'o' key in Webots,
   which sends an IMURequest message to engine"""
__author__ = "Guillermo Bautista <guillermo.bautista@anki.com>"

import argparse
import math
import sys
import os
import numpy as np
from string import ascii_letters, digits
from numpy import loadtxt

COLUMN_KEYS = {}

SUPPORTED_FILE_TYPES = ('.dat')

# These sensor types should be listed in the order they appear in the IMU raw data
# message. Please consult the IMU data message documentation before making changes.
SENSORS = ["time", "accel", "gyro"]
SENSORS_WITH_MULTIPLE_AXES = ["accel", "gyro"]

def createColumnKeys():
    COLUMN_KEYS["time"] = 0
    # The assumption is that any sensor that measures along multiple axes will
    # measure along the three standard (X/Y/Z) axes.
    axes = ['x', 'y', 'z']
    column = 1
    for sensor in SENSORS_WITH_MULTIPLE_AXES:
        COLUMN_KEYS[sensor] = {}
        for axis in axes:
            COLUMN_KEYS[sensor][axis] = column
            column += 1

def getColumnKey(sensor, axis):
    sensor_low = sensor.lower()
    if sensor_low not in COLUMN_KEYS:
        print(("Cannot access column for invalid sensor: {}").format(sensor_low))
        return -1
    elif sensor_low not in SENSORS_WITH_MULTIPLE_AXES:
        return COLUMN_KEYS[sensor_low]
    else:
        axis_low = axis.lower()
        if axis_low not in COLUMN_KEYS[sensor_low]:
            print(("Cannot access column for invalid sensor axis: {}").format(axis_low))
            return -1
        return COLUMN_KEYS[sensor_low][axis_low]

def getColumnKeys(sensor, axis = ""):
    sensor_low = sensor.lower()
    if sensor_low not in COLUMN_KEYS:
        print(("Cannot access column for invalid sensor: {}").format(sensor_low))
        return []
    if sensor_low not in SENSORS_WITH_MULTIPLE_AXES:
        return [COLUMN_KEYS[sensor_low]]
    else:
        axis_low = axis.lower()
        if axis_low not in COLUMN_KEYS[sensor_low]:
            return list(COLUMN_KEYS[sensor_low].values())
        else:
            return [COLUMN_KEYS[sensor_low][axis_low]]

def createArgParser():
    parser = argparse.ArgumentParser(description='Process some IMU data file.')
    # Add a required argument for the path to the data file.
    parser.add_argument("file", type=str, help="File to open for analysis")
    # Add optional argument to print the number of data points in the file.
    parser.add_argument("-p", "--points", help = "Get number of data points in file.",
                        action = 'store_true');
    # Add optional arguments to compute metrics on available sensors.
    for sensor in SENSORS:
        short_arg = '-' + sensor[0]
        arg = "--" + sensor
        if sensor not in SENSORS_WITH_MULTIPLE_AXES:
            # Sensors that have only a single axis of measurement require no
            # list of choices
            parser.add_argument(short_arg, arg,
                                help = ("Get metrics for {} sensor reading").format(sensor),
                                action = 'store_true')
        else:
            arg_choices = list(COLUMN_KEYS[sensor])
            # Add in a an additional option to calculate the statistics for the overall
            # magnitude of the sensor's readings.
            arg_choices.append("mag")
            parser.add_argument(short_arg, arg, type=str, nargs='+',
                                choices=arg_choices,
                                help=("Get metrics for {} axis reading").format(sensor));
    # Add optional argument to allow user to give path to output file location.
    parser.add_argument('-o', '--out', type=str, help="File to append/write results.")
    return parser

def loadDataFile(file_path_name, verbose = False):
    if os.path.isfile(file_path_name):
        lines = loadtxt(file_path_name, skiprows=1, unpack=False)
        if verbose:
            print("File data loaded into array.\n")
        return lines
    else:
        print(("Invalid file path ({}), could not load file data into array.").format(file_path_name))
        return np.zeros([0, 0])

def loadData(path):
    data = {}
    if os.path.exists(path):
        if os.path.isdir(path):
            for root, dirs, files in os.walk(path):
                for file in files:
                    if file.endswith(SUPPORTED_FILE_TYPES):
                        file_path = os.path.join(root, file)
                        data[file_path] = loadDataFile(file_path)
        elif path.endswith(SUPPORTED_FILE_TYPES):
            data[path] = loadDataFile(path)
    return data

def computeMetricForSensor(sensor, data, axis, method, metric_name):
    metric = np.nan
    col_keys = getColumnKeys(sensor, axis)
    if len(col_keys) > 0:
        if axis != "mag":
            metric = method(data[:, col_keys])
            print(("{} for {} readings along {} axis: {}").format(metric_name, sensor,
                                                                  axis, metric))
        else:
            metric = magnitudeMetric(data[:, col_keys], method)
            print(("{} for magnitude of {} readings: {}").format(metric_name, sensor,
                                                                 metric))
    return metric

def sampleStd(data):
    return np.std(data, ddof = 1)

def magnitudeMetric(data_array, method):
    return method(np.linalg.norm(data_array, axis = 1))

def computeMetricsForSensor(sensor_type, data, axes=["<>"]):
    entries = []
    if sensor_type not in SENSORS:
        print(("\nInvalid sensor type ({}), cannot compute metrics.").format(sensor_type))
    else:
        # Add empty line for readability
        print()
        for axis in axes:
            entry = {"sensor":sensor_type,
                     "avg":computeMetricForSensor(sensor_type, data, axis,
                                                  np.average, "Average"),
                     "std_dev":computeMetricForSensor(sensor_type, data, axis,
                                                      sampleStd, "Sample standard deviation")}
            if sensor_type in SENSORS_WITH_MULTIPLE_AXES:
                entry["axis"] = axis
            entries.append(entry)
    return entries

def yesNoQuestion(question, default_ans = True):
    while True:
        response = input(("{} (y/n) [{}] ").format(question,
                                                  'y' if default_ans else 'n'))
        if not response:
            return default_ans
        lower_resp = response.lower();
        if (lower_resp == 'y' or lower_resp == 'n'):
            return lower_resp == 'y'
        else:
            print("Invalid input, please respond with (y/n).")

def shouldRecordResults():
    return yesNoQuestion("\nWould you like to store the results to a file?")

def getRecordsFileName(arg_output_filename = ""):
    while True:
        possible_write_file_path_name = arg_output_filename
        if not arg_output_filename:
            possible_write_file_path_name = input("Please enter a valid file name to write to: ")
        if os.path.isfile(possible_write_file_path_name):
            if (yesNoQuestion(("Are you sure you want to edit this file ({})?")
                              .format(possible_write_file_path_name))):
                return possible_write_file_path_name
        else:
            query = "Are you sure you want to create a new file to write to at this location: "
            if (yesNoQuestion(query + ("({})?").format(possible_write_file_path_name))):
                return possible_write_file_path_name

def getIDInfo():
    serial_num_query = "Please enter a valid serial number (S/N) for the robot: "
    serial_num = input(serial_num_query)
    while not all(c in (ascii_letters + digits) for c in serial_num):
        print("Invalid serial number entered, only alphanumeric characters allowed.")
        serial_num = input(serial_num_query)
    lot_num = input("Please enter a valid lot number for the robot: ")
    model_num = input("Please enter a valid model number (M/N) for the robot: ")

    id_info = {"S/N":serial_num, "Lot":lot_num, "M/N":model_num};
    for key, number in id_info.items():
        if not number:
            id_info[key] = "<>"
    return id_info

def GetTempInfo():
    query = "Enter the reported IMU temperature in degrees Celsius at the time of logging: "
    temp = input(query)
    while not all(c in digits for c in temp):
        print("Invalid temperature entered, only numerical characters allowed.")
        temp = input(query)
    if not temp:
        return "<>"
    return temp

def writeDataEntriesToFile(file_path_name, data_entries):
    with open(file_path_name, 'a') as file_handler:
        id_info = getIDInfo()
        # TODO: Delete this once the robot's temperature is reported in the log.
        temp_during_log_degC = GetTempInfo()
        file_handler.write(("\n\nRobot ID: S/N {}, Lot: {}\n").format(id_info["S/N"], id_info["Lot"]))
        file_handler.write(("M/N: {}\n").format(id_info["M/N"]))
        for entry in data_entries:
            if not entry:
                continue
            else:
                writeDataEntryToFile(file_handler, entry)
        file_handler.write(("Temperature: {} degrees Celsius").format(temp_during_log_degC))

def roundToSigFigs(x, sig):
    return round(x, sig - int(math.floor(math.log10(abs(x)))) - 1)

def getRotationalConversions(mrad_per_sec):
    mradps = roundToSigFigs(mrad_per_sec, 4)
    rpm = roundToSigFigs((mrad_per_sec / 1000.0) * 60.0 / (2 * math.pi), 4)
    degps = roundToSigFigs(math.degrees(mrad_per_sec / 1000.0), 4)
    return {"mrad/s":mradps, "rpm": rpm, "deg/s": degps}

def writeDataEntryToFile(file_handler, data_entry):
    line1 = []
    line2 = []
    line1.append(("{}").format(data_entry["sensor"].capitalize()))
    if data_entry["sensor"] in SENSORS_WITH_MULTIPLE_AXES:
        if data_entry["axis"] == "mag":
            line1.append("Magnitude Reading:")
        else:
            line1.append(("({}-Axis) Reading:").format(data_entry["axis"].upper()))
        if data_entry["sensor"] == "gyro":
            avg_conv = getRotationalConversions(data_entry["avg"])
            avg_str = ("{} mrad/s ({} RPM) ({} deg/s)").format(avg_conv["mrad/s"],
                                                               avg_conv["rpm"],
                                                               avg_conv["deg/s"])
            line1.append(avg_str)
            line1.append("+/- 1 mrad/s (0.06 deg/s)\n")
            file_handler.write(' '.join(line1))

            std_dev_conv = getRotationalConversions(data_entry["std_dev"])
            line2.append("Sample standard deviation:")
            std_dev_str = ("{} mrad/s ({} RPM) ({} deg/s)\n").format(std_dev_conv["mrad/s"],
                                                                     std_dev_conv["rpm"],
                                                                     std_dev_conv["deg/s"])
            line2.append(std_dev_str)
            file_handler.write(' '.join(line2))
        elif data_entry["sensor"] == "accel":
            print(("No logic to write {} data entry to file, skipping.").format(data_entry["sensor"]))
            # TODO: Write code to log accelerometer data entries.
    else:
        print(("No logic to write {} data entry to file, skipping.").format(data_entry["sensor"]))
        # line1.append("reading:")
        # if data_entry["sensor"] == "time":
            # TODO: Write code to log time data entries.
        # TODO: Write code to log other sensor data entries.


def main():
    createColumnKeys()
    parser = createArgParser()
    args = parser.parse_args()
    data_dict = loadData(args.file)
    if not data_dict:
        print("No data found in file path provided. Exiting.")
        return
    print(("{} files with {} extensions parsed from file path provided.").format(len(data_dict), SUPPORTED_FILE_TYPES))
    should_record_results = False
    for file_path, dataset in data_dict.items():
        data_entries = []
        # Iterate over all optional parameters and compute metrics for the sensors/axes
        # selected by the optional parameters passed through.
        print(("\nComputing metrics for new log dataset from file: {}").format(file_path))
        for opt_param in parser._optionals._group_actions:
            opt_param_name = opt_param.dest
            if opt_param_name in SENSORS and getattr(args, opt_param_name):
                sensor_axes = set(getattr(args, opt_param_name))
                data_entries.extend(computeMetricsForSensor(opt_param_name, dataset, sensor_axes))
        if args.points:
            print(("Number of readings recorded in log: {}").format(data.shape[0]))
        print(("\nProcessed new log dataset from file: {}").format(file_path))
        if should_record_results or args.out or shouldRecordResults():
            should_record_results = True
            if args.out:
                print("Attempting to cache results to output file.")
            writeDataEntriesToFile(getRecordsFileName(args.out), data_entries)

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\nInterrupted")
        try:
            sys.exit(0)
        except SystemExit:
            os._exit(0)
