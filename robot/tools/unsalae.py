import csv
from ast import literal_eval as make_tuple

fr = open("untitled2.csv")

mode = "header"
slug = ""
for line in fr:
    row = line.split(",")
    if mode == "header":
        mode = "seek"
    else:
        char = row[2].split(" ")[0]

        # if char=="'170'":
        #     if slug[8:10] == "dc":
        #         print(slug)
        #     slug = ""
        # slug = slug+char
        print(char,end="")
        if char=="\\n":
            print()





                # print(row)
        # print(row[2])
        # print(row[2].split(" "))
#        char = make_tuple(row[2].split(" ")[-1])
