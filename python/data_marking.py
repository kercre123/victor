#!/usr/bin/python

"""data_marking.py

Uses the input .srt file to mark the input data file per line as tagged or not.

Author: Lee Crippen
10-14-2015
"""

import os, argparse, argparse_help, datetime, sys, pysrt

""" Main function sets up the args, parses them to make sure they're valid, then uses them """
def main():
    parser = argparse.ArgumentParser(description='Use the input subtitle file (.srt) to mark the input data per line (1 for the matching data line, 0 otherwise).',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('srtFile', help='Subtitle format file to use for specifying marked sections.', type=file)
    parser.add_argument('dataFile', help='The data file to be marked.', type=file)
    parser.add_argument('-o', help='Output directory for marked files. Default is same directory as input data.', type=argparse_help.is_directory_writable, nargs='?', dest='outDir', default=None)
    parser.add_argument('--data-sync', help='Time associated with first data event to match with first subtitle mark.', type=float, default=0)

    args = parser.parse_args()

    """ Figure out where we'll put the output """
    outDir = args.outDir
    if outDir == None:
        outDir = os.path.dirname(args.dataFile.name)

    """ Get a list of subtitle objects from our input subtitle file """
    subs = pysrt.open(args.srtFile.name)
    if len(subs) > 0:
        """ Use passed in time of the first event in data to calculate the offset we'll use """
        timeOffset = args.data_sync - get_time_seconds(subs[0].start.to_time())

        """ Set up the filename and file object for the output file """
        splitName = os.path.basename(args.dataFile.name).split('.')
        newFileName = splitName[0] + "_m"
        if len(splitName) > 1:
            newFileName += '.' + '.'.join(splitName[1:])
        outFile = open(outDir + os.sep + newFileName, 'wb')

        process_data(subs, args.dataFile, outFile, timeOffset)
        outFile.close()

def process_data(subs, dataFile, outFile, dataSync):
    line = dataFile.readline()
    for sub in subs:
        subTimeStart = dataSync + get_time_seconds(sub.start.to_time())
        subTimeEnd = dataSync + get_time_seconds(sub.end.to_time())
        while line != '' and len(line) > 2:
            """ We expect the first tab-delimited token in each line of data to be a float representing time in seconds """
            nextTime = float(line.split('\t')[0])

            """ If we've reached the end of the time range, mark 0 and break to go to next """
            if nextTime > subTimeEnd:
                outFile.write(line[:(len(line)-2)] + "\t0\n")
                line = dataFile.readline()
                break;
            elif nextTime >= subTimeStart:
                """ If we're inside of a range, mark 1 """
                outFile.write(line[:(len(line)-2)] + "\t1\n")
                line = dataFile.readline()
            else:
                """ Otherwise mark 0 """
                outFile.write(line[:(len(line)-2)] + "\t0\n")
                line = dataFile.readline()

    """ Once we've run through all the time ranges just copy out the rest of the data marked with 0 """
    while line != '':
        if len(line) > 2:
            outFile.write(line[:(len(line)-2)] + "\t0\n")
        line = dataFile.readline()


def get_time_seconds(time):
    delta = datetime.timedelta(hours=time.hour, minutes=time.minute, seconds=time.second, microseconds=time.microsecond)
    return delta.total_seconds()

if __name__ == "__main__":
    main()
