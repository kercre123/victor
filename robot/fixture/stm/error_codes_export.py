#!python
#export error code list from source header to a CSV file

infile = "app/fixture.h"
outfile = "error_codes.csv"
parsed_lines = 0
num_ecodes = 0

def ecode_split(line):
  name = line.split()[1]
  val = line.split()[2]
  comment = ""
  if "//" in line:
    comment = line.split("//")[1].strip()
  return name, val, comment

def parse_line(line, fout):
  global num_ecodes, parsed_lines
  if line.startswith("//<export heading>"):
    line = line[len("//<export heading>"):].strip()
    if len(line) > 0:
      fout.write( "\n{}\n".format(line) )
  elif line.startswith("#define ERROR_"):
    num_ecodes += 1
    [name,val,comment] = ecode_split(line) #split line into the parts we care about
    fout.write("{},{},  {}\n".format( name.ljust(15,' '), val.ljust(3,' ') ,comment ) )
  parsed_lines+=1
  return

export = 0
with open(infile, 'r+') as fin:
  with open(outfile, "w") as fout:
    for line in fin:
      if line.startswith("//<export start>"):
        export = 1
      elif line.startswith("//<export end>"):
        export = 0
      elif export == 1: #only parse lines in the error-code region (easier)
        parse_line( line.rstrip(), fout )

print( "found {} error codes. parsed {} lines from {}".format(num_ecodes, parsed_lines, infile) )
exit()
