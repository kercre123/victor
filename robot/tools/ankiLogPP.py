#!/usr/bin/env python3
from __future__ import print_function
"""
AnkiLog Pre-Processor
The AnkiLog pre-processor has two modes, both of which process all the source (*.{h,hpp,c,cpp}) files in the specified
directory.

In preprocessor mode, it finds AnkiLog macros and generates string IDs for the strings and modifies the macro calls to
include the string IDs.

In string table mode, it finds the files modified by the pre-processor mode and generates a JSON file containing the
string tables nessisary to format data sent via the macros.

Both modes may be run in one invocation.

Copyright 2015 Anki Inc. All rights reserved.
"""
__author__  = "Daniel Casner <daniel@anki.com>"
__version__ = "0.1"

import sys, os, re, argparse, json, mmap

if sys.version_info.major < 3:
    sys.stderr.write("Python 2x is depricated" + os.linesep)

verbosity = 0
DEFAULT_SOURCE_FILE_TYPES = ['.h', '.c', '.hpp', '.cpp']

ASSERT_NAME_ID = 0 # All asserts have a name ID of 0

MAX_VAR_ARGS = 12

def vPrint(level, text, err=False):
    global verbosity
    if verbosity >= level:
        out = sys.stderr if err else sys.stdout
        out.write("{tab:s}{text:s}{linesep:s}".format(text=text, linesep=os.linesep, tab="\t"*level))

def assertString(assertCondition, code):
    "Returns a format string for a given assert macro"
    return 'Failed "{:s}" in file "{:s}" line %d'.format(assertCondition.decode(), code.fileName)

def looksLikeInt(s):
    return s.strip().isdigit()

def looksLikeString(s):
    x = s.strip()
    return x.startswith(b'"') and x.endswith(b'"')

def rangeGenerator(start, end, step=1):
    "A real generator for yielding range numbers"
    i = start
    while i < end:
        yield i
        i += 1

class LogPPError(Exception):
    pass

class CoordinatedFile:
    "A class wrapper for managing files, reads the whole thing in and keeps track of different coordinates"
    def __init__(self, fileName, writeAccess=False):
        if not os.path.isfile(fileName):
            raise IOError("\"{}\" is not a file".format(fileName))
        else:
            self.fileName = fileName
            self.writeAccess = writeAccess
            try:
                if writeAccess:
                    self.fh = open(fileName, "r+b")
                    self.__memoryMap()
                    self.adjustedLength = len(self.contents)
                else:
                    self.fh = open(fileName, "rb")
                    self.__memoryMap()
            except Exception as e:
                sys.exit("Error while trying to memory map file \"{}\":{linesep}\t{}{linesep}".format(fileName, str(e), linesep=os.linesep))
            self.lineIndecies = [0]
            position = self.contents.find(b"\n")
            while position >= 0:
                self.lineIndecies.append(position)
                position = self.contents.find(b"\n", position+1)
            self.index = 0

    def __memoryMap(self):
        if self.writeAccess:
            self.contents = mmap.mmap(self.fh.fileno(), 0)
        else:
            self.contents = mmap.mmap(self.fh.fileno(), 0, access=mmap.ACCESS_READ)

    def __del__(self):
        if self.writeAccess:
            self.contents.flush()
        self.contents.close()
        self.fh.close()

    def update(self, index=0, line=0):
        "Update the pointer by index and / or line"
        self.index = self.lineIndecies[line] + index

    @property
    def line(self):
        "Get the line the index we are at"
        for l, count in enumerate(self.lineIndecies):
            if count > self.index:
                return l-1

    def __repr__(self):
        "A debugging representation of the state"
        l = self.line
        return "CoordinatedFile('{}' [line {}, char {}])".format(self.fileName, l, self.index - self.lineIndecies[l])

    def __str__(self):
        "A nice format string showing were we are in the file"
        l = self.line
        c = self.index - self.lineIndecies[l]
        return '"{filename}" line {l}, character {c}:{linesep}{line}{linesep}{dots}^{linesep}'.format(
            filename=self.fileName,
            l = l, c = c,
            linesep = os.linesep,
            line = self.contents[self.lineIndecies[l]:self.lineIndecies[l+1]].decode(),
            dots = '.' * c
        )

    def __getitem__(self, index):
        "Get an item relative to the current index in the file"
        return bytes([self.contents[index + self.index]])

    def __next__(self):
        "Get the next item in the file"
        ret = bytes([self.contents[self.index]])
        self.index += 1
        return ret

    def search(self, regex):
        "Advance the index past the next occurance of the specified regular expression, returns True or False"
        m = regex.search(self.contents[self.index:])
        if m is None:
            return False
        else:
            self.index += m.end()
            return True

    def reset(self):
        "Reset the pointer to the begining"
        self.update(0)

    def write(self):
        "Write (modified) contents back out to file"
        if not self.writeAccess:
            raise ValueError("This CoordinatedFile ({}) instance doesn't have write access".format(self.fileName))
        else:
            self.contents.flush()
            self.fh.seek(self.adjustedLength, 0)
            self.fh.truncate()

    def expand(self, addition):
        "Makes the file bigger and re-memory maps it"
        if not self.writeAccess:
            raise ValueError("This Coordinated file ({}) does not have write access".format(self.fileName))
        else:
            self.contents.close()
            self.fh.seek(0, 2) # Seek the end of the file
            self.fh.write((os.linesep * addition).encode())
            self.fh.seek(0, 0) # Seek back to the beginning of the file
            self.__memoryMap()


    def insert(self, slice, newText):
        "Replace the slice from start to end with newText and return the change in length"
        if not self.writeAccess:
            raise ValueError("This Coordinated file ({}) does not have write access".format(self.fileName))
        start, end = slice
        lenChange = len(newText) - (end-start)
        if lenChange == 0:
            self.contents[start:end] = newText
        else:
            if lenChange > 0:
                self.expand(lenChange)
            self.contents.move(end + lenChange, end, self.adjustedLength - end)
            self.contents[start:end+lenChange] = newText
            self.adjustedLength += lenChange
            if self.index > start: self.index += lenChange
        return lenChange

class ParseInstance:
    "Stores data about a parsed macro instance"
    def __init__(self, mangleStart, mangleEnd, nameId, name, fmtId, fmt, nargs):
        self.mangleStart = mangleStart
        self.mangleEnd   = mangleEnd
        self.nameId      = nameId
        self.name        = name
        self.fmtId       = fmtId
        self.fmt         = fmt
        self.nargs       = nargs
    def __repr__(self):
        return "ParseInstance(mangleStart={}, mangleEnd={}, nameId={}, name={}, fmtId={}, fmt={}, nargs={})".format(
            repr(self.mangleStart),
            repr(self.mangleEnd),
            repr(self.nameId),
            repr(self.name),
            repr(self.fmtId),
            repr(self.fmt),
            repr(self.nargs))
    @property
    def length(self):
        return self.mangleEnd - self.mangleStart
    def getMangle(self, offset=0):
        "Return the mangle indecies with an optional offset applied"
        return (self.mangleStart + offset, self.mangleEnd + offset)

class ParseParams:
    "Class container for parsing a macro"
    CONTAINERS = {
        b"'": b"'",
        b'"': b'"',
        b'(': b')',
        b'[': b']',
        b'{': b'}',
        }
    FORMATTER_KEY = re.compile(r'(?<!%)%[^%]') # Find singal % marks
    FORMAT_BLACK_LIST = re.compile(r'%[bsl]')
    def __init__(self, preArgs=0):
        "Store macro parsing parameters"
        self.preArgs = preArgs # Number of arguments we don't mess with and just let the pre-processor handle
    def __repr__(self):
        return "ParseParams({:d})".format(self.preArgs)
    def parseArgs(self, code):
        "Just parse the args from the code"
        depth = []
        preArgs = []
        args = []
        a = b""
        startInd = code.index
        mangleStart = startInd if self.preArgs == 0 else None
        for ch in code:
            if not depth: # things that can only happen at depth 0
                if ch in (b',', b')'):
                    if len(preArgs) < self.preArgs:
                        preArgs.append(a)
                        if len(preArgs) == self.preArgs:
                            mangleStart = code.index + sum([len(pa) + 1 for pa in preArgs])
                            if ch == b')': mangleStart -= 1
                    else:
                        args.append(a)
                    a = b""
                    if ch == b")":
                        break
                    else:
                        continue
            a += ch
            if depth and depth[-1] in (b"'", b'"') and code[-2] != b"\\": # Sadly quotes require separate handling first because other characters inside a quote do not count
                if ch == self.CONTAINERS[depth[-1]]: # Match for current
                    depth.pop(-1)
            elif ch in self.CONTAINERS.keys():
                depth.append(ch)
            elif depth and ch == self.CONTAINERS[depth[-1]]: # Match for current
                depth.pop(-1)
        else:
            code.update(index = startInd)
            vPrint(1, "preArgs = {:s}\targs = {:s}\tdepth = {:s}".format(repr(preArgs), repr(args), repr(depth)))
            raise LogPPError("Ran out of code without finding end of logging macro")
        return mangleStart, preArgs, args
    def consume(self, code):
        """Reads from the string code buffer to parse arguments. Updates coordinates pointer and returns ParseInstance"""
        mangleStart, preArgs, args = self.parseArgs(code)
        if len(args) >= 5 and \
            looksLikeInt(args[0]) and looksLikeString(args[1]) and \
            looksLikeInt(args[2]) and looksLikeString(args[3]) and \
            looksLikeInt(args[4]): # This is an already processed macro instance
            mangleEnd = mangleStart + sum([len(a)+1 for a in args[:5]])-1
            nameId = int(args[0])
            name   = args[1].strip()[1:-1].decode()
            fmtId  = int(args[2])
            fmt    = args[3].strip()[1:-1].decode()
            nargs = int(args[4])
            numFormatArgs = len(self.FORMATTER_KEY.findall(fmt))
            badFormatArgs = self.FORMAT_BLACK_LIST.findall(fmt)
            if badFormatArgs:
                raise LogPPError("Format arguments {} are not supported".format(repr(badFormatArgs)))
            elif nargs != numFormatArgs:
                raise LogPPError("Format string \"{}\" expects {} arguments but have {}".format(fmt, numFormatArgs, nargs))
            elif nargs > MAX_VAR_ARGS:
                raise LogPPError("Too many var args to AnkiLogging macro")
            else:
                if len(args) - 5 == nargs: # All good
                    return ParseInstance(mangleStart, mangleEnd, nameId, name, fmtId, fmt, nargs)
                else: # Format must have changed, regenerate it
                    return ParseInstance(mangleStart, mangleEnd, nameId, name, None, fmt, len(args) - 5)
        elif len(args) >= 2 and looksLikeString(args[0]) and looksLikeString(args[1]): # An unprocessed macro
            mangleEnd = mangleStart + len(args[0]) + 1 + len(args[1])
            nargs = len(args) - 2
            name = args[0].strip()[1:-1].decode()
            fmt  = args[1].strip()[1:-1].decode()
            numFormatArgs = len(self.FORMATTER_KEY.findall(fmt))
            badFormatArgs = self.FORMAT_BLACK_LIST.findall(fmt)
            if badFormatArgs:
                sys.exit("Format arguments {} are not supported".format(repr(badFormatArgs)))
            elif nargs != numFormatArgs:
                raise LogPPError("Format string \"{}\" expects {} arguments but have {}".format(fmt, numFormatArgs, nargs))
            elif nargs > MAX_VAR_ARGS:
                raise LogPPError("Too many var args to AnkiLogging macro")
            else:
                return ParseInstance(mangleStart, mangleEnd, None, name, None, fmt, nargs)
        else:
            vPrint(1, repr(args))
            raise LogPPError("Bad AnkiLogging macro arguments")

class AssertParseParams(ParseParams):
    """Parse parameters specific for an assert macro instance."""
    def __init__(self):
        ParseParams.__init__(self, 1)
    def consume(self, code):
        """Reads from the string code buffer to parse arguments, Updates coordinates pointer and returns ParseInstance"""
        mangleStart, preArgs, args = self.parseArgs(code)
        fmt = assertString(preArgs[0], code)
        if len(args) == 0: # An unprocessed macro
            return ParseInstance(mangleStart, mangleStart, ASSERT_NAME_ID, "ASSERT", None, fmt, 1)
        elif len(args) == 1 and looksLikeInt(args[0]): # An already processed macro instance
            mangleEnd = mangleStart + len(args[0])
            return ParseInstance(mangleStart, mangleEnd, ASSERT_NAME_ID, "ASSERT", int(args[0]), fmt, 1)
        else:
            raise LogPPError("Bad AnkiAssert macro arguments")


class ParseData:
    "A class to collect the parsed data from a directory of source files"

    INCLUDE_FILE_RE = re.compile(r'#include.+logging.h"')

    def __init__(self, directories, sourceTypes):
        "Parse all files of type in sourceTypes"
        self.pt = {} # Parse table
        for directory in directories:
            vPrint(0, "Parsing files in \"{}\"".format(directory))
            for dirpath, dirs, files in os.walk(directory):
                for file in files:
                    base, ext = os.path.splitext(file)
                    if ext in sourceTypes:
                        fpn = os.path.join(dirpath, file)
                        fh = open(fpn, "r")
                        try:
                            for line in fh:
                                if self.INCLUDE_FILE_RE.search(line):
                                    self.pt[fpn] = self.parseFile(fpn)
                                    break
                            else:
                                vPrint(2, "Skipping file \"{}\" because it doesn't have the logging include.".format(fpn))
                        except Exception as e:
                            sys.exit("Error parsing file \"{}\":{linesep}\t{}{linesep}".format(file, str(e), linesep=os.linesep))
                        fh.close()
            self.nameTable  = {ASSERT_NAME_ID: "ASSERT", 1: "wifi.dropped_traces", 2: "rtip.dropped_traces", 3: "body.dropped_traces"}
            self.fmtTable   = {0: ("Invalid format ID", 0), 1: ("dropped %d traces", 1)}
            self.dirtyFiles = set()

    MACROS = {
        re.compile(b"AnkiEvent\\s*\\("): ParseParams(),
        re.compile(b"AnkiInfo\\s*\\("):  ParseParams(),
        re.compile(b"AnkiDebug\\s*\\("): ParseParams(),
        re.compile(b"AnkiDebugPeriodic\\s*\\("): ParseParams(1),
        re.compile(b"AnkiWarn\\s*\\("):  ParseParams(),
        re.compile(b"AnkiError\\s*\\("): ParseParams(),
        re.compile(b"AnkiConditionalError\\s*\\("): ParseParams(1),
        re.compile(b"AnkiConditionalErrorAndReturn\\s*\\("): ParseParams(1),
        re.compile(b"AnkiConditionalErrorAndReturnValue\\s*\\("): ParseParams(2),
        re.compile(b"AnkiConditionalWarn\\s*\\("): ParseParams(1),
        re.compile(b"AnkiConditionalWarnAndReturn\\s*\\("): ParseParams(1),
        re.compile(b"AnkiConditionalWarnAndReturnValue\\s*\\("): ParseParams(2),
        re.compile(b"AnkiAssert\\s*\\("): AssertParseParams()
    }

    def parseFile(self, file):
        "Parse a single file and return meta-data for it"
        vPrint(2, "Parsing file \"{}\"".format(file))
        code = CoordinatedFile(file)
        instances = []
        try:
            for m, pp in self.MACROS.items():
                vPrint(3, "Processing macro {0:s}: {1:s}".format(str(m), str(pp)))
                while code.search(m):
                    instances.append(pp.consume(code))
                code.reset()
        except LogPPError as err:
            sys.exit("{err:s}{linesep}{coord:s}{linesep}".format(err=str(err), coord=str(code), linesep=os.linesep))
        # Sort put instances in order from start to finish
        if sys.version_info.major < 3:
            instances.sort(cmp=lambda a, b: cmp(a.mangleStart, b.mangleStart))
        else:
            instances.sort(key=lambda x: x.mangleStart)
        # Make sure we haven't wound up with overlapping instances some how
        for i in range(1, len(instances)):
            assert instances[i-1].mangleEnd < instances[i].mangleStart
        return instances

    def reconsile(self, shouldBeComplete=False, purge=False):
        "Reconsiles the string tables from all the input table"
        vPrint(1, "Reconsiling string tables." + (" Expecting complete" if shouldBeComplete else ""))
        if purge: vPrint(2, "Purging string tables")
        # First pass, add things that already have IDs and mark ones with duplicates
        for fpn, instances in self.pt.items():
            if purge: # If just clearing everything, go through and reset
                for i in instances:
                    i.nameId = None
                    i.fmtId  = None
            else:
                for i in instances:
                    if i.nameId is not None:
                        if i.nameId not in self.nameTable:
                            self.nameTable[i.nameId] = i.name
                        else: # If ID is already in table
                            if i.name != self.nameTable[i.nameId]: # But not a match
                                if shouldBeComplete:
                                    raise LogPPError("Expecting complete but found conflicting name ID in file \"{}\"".format(fpn))
                                else:
                                    i.nameId = None # So need new ID
                    elif shouldBeComplete:
                        raise LogPPError("Expected complete but file \"{}\" had an unprocessed macro".format(fpn))
                    if i.fmtId is not None:
                        if i.fmtId not in self.fmtTable:
                            self.fmtTable[i.fmtId] = (i.fmt, i.nargs)
                        else: # If ID is already in table
                            if i.fmt != self.fmtTable[i.fmtId][0]: # But not a match
                                if shouldBeComplete:
                                    raise LogPPError("Expecting complete but found conflicting format ID in file \"[]\"".format(fpn))
                                else:
                                    i.fmtId = None
                    elif shouldBeComplete:
                        raise LogPPError("Expecting complte but file \"{}\" has an unprocessed macro.".format(fpn))
        # Second pass, give values to things which don't have them
        if shouldBeComplete: # If shouldBeComplete was set, we covered this on the first pass
            return # So just return
        else:
            # Generators for new IDs
            nameIdGen = rangeGenerator(max(self.nameTable.keys()) + 1, 2**16-1)
            fmtIdGen  = rangeGenerator(max(self.fmtTable.keys())  + 1, 2**32-1)
            # Inverse mappings to find existing ones
            nameIdTable = {v: k for k, v in self.nameTable.items()}
            fmtIdTable  = {v[0]: k for k, v in self.fmtTable.items()}
            for fpn, instances in self.pt.items():
                for i in instances:
                    if i.nameId is None:
                        if i.name in nameIdTable:
                            i.nameId = nameIdTable[i.name]
                        else:
                            i.nameId = next(nameIdGen)
                            self.nameTable[i.nameId] = i.name
                            nameIdTable[i.name] = i.nameId
                        self.dirtyFiles.add(fpn)
                    if i.fmtId is None:
                        if i.fmt in fmtIdTable:
                            i.fmtId = fmtIdTable[i.fmt]
                        else:
                            i.fmtId = next(fmtIdGen)
                            self.fmtTable[i.fmtId] = (i.fmt, i.nargs)
                            fmtIdTable[i.fmt] = i.fmtId
                        self.dirtyFiles.add(fpn)

    def doMangling(self):
        "Update all the files in the directory we're recursing over"
        vPrint(1, "Source files in need of update are: {}".format(', '.join(self.dirtyFiles)))
        for fpn in self.dirtyFiles:
            code = CoordinatedFile(fpn, True)
            accumulatedOffset = 0
            for inst in self.pt[fpn]:
                if inst.nameId == ASSERT_NAME_ID: # Special handling for assert macros
                    if inst.length == 0:
                        insert = ", {:d}".format(inst.fmtId).encode()
                    else:
                        insert = " {:d}".format(inst.fmtId).encode()
                    accumulatedOffset += code.insert(inst.getMangle(accumulatedOffset), insert)
                else:
                    insert = " {nameId:d}, \"{name:s}\", {fmtId:d}, \"{fmt:s}\", {nargs:d}".format(**inst.__dict__).encode()
                    accumulatedOffset += code.insert(inst.getMangle(accumulatedOffset), insert)
            code.write()

    def writeJSON(self, fileName):
        "Write string tables out to JSON file"
        encoder = json.JSONEncoder(indent=4)
        fh = open(fileName, "w")
        fh.write(encoder.encode({"nameTable": self.nameTable, "formatTable": self.fmtTable}))
        fh.close()

def main(argv):
    """Main entry point, allows specifying an alternate argv"""
    global verbosity
    parser = argparse.ArgumentParser(prog="AnkiLogPP", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-v', '--verbose', default=0, action='count', help="Increase the verbosity of the parser")
    parser.add_argument('-p', '--preprocessor', action='store_true', help="Run the preprocessor mode")
    parser.add_argument('-t', '--string-table', action='store_true', help="Run the string table generator")
    parser.add_argument('-o', '--output', default="AnkiLogStringTables.json", help="Specify an alternate output for the string table")
    parser.add_argument('-s', '--source-type', action='append', help="Specify alternate source type(s)")
    parser.add_argument('--purge', action='store_true', help="CAUTION! Purge all string IDs and start fresh")
    parser.add_argument('dirs', nargs="+", help="Directory to parse")
    args = parser.parse_args(argv[1:])
    if args.source_type is None:
        args.source_type = DEFAULT_SOURCE_FILE_TYPES
    else:
        args.source_type = [t if t.startswith('.') else '.' + t for t in args.source_type]
    for dir in args.dirs:
        if not os.path.isdir(dir):
            sys.exit("dirs argument must be a directory, \"{}\" does not appear to be one".format(os.path.join(os.path.curdir, dir)))
    verbosity = args.verbose
    vPrint(1, "Will be verbose to {}".format(verbosity))
    expectingComplete = args.string_table and not args.preprocessor
    if args.purge:
        if expectingComplete:
            sys.exit("Running only the string table generator without preprocessor with --purge doesn't make sense, aborting.")
        elif input("Are you sure you want to purge all existing table IDs? [y/N] ").lower() != 'y':
            sys.exit("Aborting")
    parse = ParseData(args.dirs, args.source_type)
    parse.reconsile(expectingComplete, args.purge)
    if args.preprocessor:
        parse.doMangling()
    if args.string_table:
        parse.writeJSON(args.output)

def importTables(fileName):
    "Import the string table JSON file and return Python dicts"
    jsonDict = json.load(open(fileName, "r"))
    nameTable = {int(k): v for k,v in jsonDict['nameTable'].items()}
    fmtTable  = {int(k): tuple(v) for k,v in jsonDict['formatTable'].items()}
    return nameTable, fmtTable

if __name__ == '__main__':
    main(sys.argv)
