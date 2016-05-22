#!/usr/bin/python

# script which reads in an sbpl format .mprim file and outputs a json
# version for use with the planner

import argparse
import json
import sys
import collections

parser = argparse.ArgumentParser(description='Converts motion primitives from and SBPL-format .mprim to an Anki format .json')
parser.add_argument('input', metavar='inputFile.mprim',
                    help='input file in sbpl format')
parser.add_argument('--rescale', default=None, metavar='F', type=float,
                    help='re-scale the cell resolution to F mm')
parser.add_argument('--output', default=None, metavar='outputFile.json',
                    help='Output json file (defaults to stdout)')
parser.add_argument('--auto_name', default=False, const=True,
                    action='store_const',
                    help='try to automatically come up with a human readable name for each action')

args = parser.parse_args()

totalNum = None

# This function returns a float cost which is the distance of the
# passed in path
def ComputeActionLength(poses):
    return 0.0

# read the file and return a dictioary which matches the json format
def ReadSBPL(filename):
    J = {}
    factor = None
    currPrim = {}
    currPrimAngle = -1
    with open(filename) as file:
        while True:
            line = file.readline().strip()
            if line == "":
                break

            tok = line.split(':')
            key = tok[0]

            if key == 'resolution_m':
                oldRes_m = float(tok[1])
                # convert to mm and also optionally change the resolution
                newRes_mm = oldRes_m * 0.001
                factor = newRes_mm / oldRes_m
                J["resolution_mm"] = newRes_mm

            elif key == 'numberofangles':
                numAngles = int(tok[1])
                J["num_angles"] = numAngles

                J["angles"] = []
                for i in range(numAngles):
                    J["angles"].append({'starting_angle': i, "prims": []})

            elif key == 'totalnumberofprimitives':
                totalNumPrims = int(tok[1])

            elif key == 'primID': # primitives always start with this
                primID = int(tok[1])
                # reset current primitive definition
                currPrim = {}
                currPrim["prim_id"] = int(tok[1])

            elif key == 'startangle_c':
                currPrimAngle = int(tok[1])

            elif key == 'endpose_c':
                tok2 = tok[1].strip().split(' ')
                pose = {}
                pose["x"] = int(tok2[0])
                pose["y"] = int(tok2[1])
                pose["theta"] = int(tok2[2])
                currPrim["end_pose"] = pose

            elif key == 'additionalactioncostmult':
                currPrim["extra_cost_factor"] = float(tok[1])

            elif key == 'intermediateposes':
                numPoses = int(tok[1])
                poses = []
                for n in range(numPoses):
                    poseStr = file.readline().strip().split(' ')
                    poses.append({"x_mm": float(poseStr[0]) * factor,
                                  "y_mm": float(poseStr[1]) * factor,
                                  "theta_rad": float(poseStr[2])})
                currPrim["intermediate_poses"] = poses

                # this is the last thing in the primitive, so add it to
                # the json
                J["angles"][currPrimAngle]["prims"].append(currPrim)

    return J

def RescaleResolution(J, newRes_mm):
    factor = newRes_mm / J["resolution_mm"]
    J["resolution_mm"] = newRes_mm
    for angle in J["angles"]:
        for prim in angle["prims"]:
            for pose in prim["intermediate_poses"]:
                pose["x_mm"] = pose["x_mm"] * factor
                pose["y_mm"] = pose["y_mm"] * factor

def AutoNameActions(J):
    
    threeCellSquared = 9*J["resolution_mm"] * J["resolution_mm"]

    for angle in J["angles"]:
        for prim in angle["prims"]:
            lastPose = prim["intermediate_poses"][-1]
            dTheta = lastPose["theta_rad"] - prim["intermediate_poses"][0]["theta_rad"]

            if abs(dTheta) < 1e-10:
                turn = "straight"
            elif dTheta < 0:
                turn = "right"
            else:
                turn = "left"

            # less than 3 cells is short, large is long, 0 is "in place"
            distSquared = lastPose["x_mm"]*lastPose["x_mm"] + lastPose["y_mm"]*lastPose["y_mm"]

            if distSquared < 1e-10:
                distance = "in place"
            elif distSquared < threeCellSquared:
                distance = "short"
            else:
                distance = "long"

            if lastPose["x_mm"] <= 0 and lastPose["y_mm"] <= 0 and distSquared > 1e-10:
                back = "backup "
            else:
                back = ""

            prim["name"] = back + distance + ' ' + turn
                

J = ReadSBPL(args.input)

if args.rescale != None:
    RescaleResolution(J, args.rescale)
             
if args.output == None:
    out = sys.stdout
else:
    out = open(args.output, 'w')

if args.auto_name:
    AutoNameActions(J)
   
json.dump(J, out, indent=2, separators=(',', ': '), sort_keys=True)
