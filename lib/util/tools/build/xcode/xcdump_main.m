//
//  main.m
//  xcdump
//
//  Created by Brian Chapados on 5/8/14.
//  Copyright (c) 2014 EXPANZ. All rights reserved.
//

#import <stdio.h>
#import <stdint.h>
#import <limits.h>
#import <getopt.h>

#import <Foundation/Foundation.h>
#import <XcodeEditor/XCProject.h>
#import <XcodeEditor/XCTarget.h>
#import <XcodeEditor/XCSourceFile.h>

// NOTE: compile this utility as a statically linked
// executable to avoid installing additional runtime dependencies.

// Dev Requirements:
// requires XcodeEditor to be installed in one of the Frameworks paths
// https://github.com/jasperblues/XcodeEditor

// To compile with dynamic linking:
// clang -framework Foundation -framework XcodeEditor  -o xcproj xcproj.m

// flag vars
int do_headers, do_help, do_verbose;

static char const PROGRAM_NAME[] = "xcdump";
static char const VERSION[] = "1.0";

char relative_path[MAXPATHLEN];
char project[MAXPATHLEN];
char target[MAXPATHLEN];

static const struct option Options [] = {
  // { "headers",      no_argument,       & do_headers,        1   }, /* TODO: support headers */
  { "project",      required_argument, NULL,                'p' },
  { "target",       required_argument, NULL,                't' },
  { "root-path",    required_argument, NULL,                'r' },
  { "help",         no_argument,       & do_help,           'h' },
  { "verbose",      no_argument,       & do_verbose,        1   },
  { 0, 0, 0, 0 }
};

void print_usage(void);
int parse_args(int argc, char * const argv[]);

void print_usage(void)
{
  printf("%s v%s Copyright (C) 2014 Anki, Inc.\n\n", PROGRAM_NAME, VERSION);
  printf("Author: Brian Chapados <chapados@anki.com>\n");
  printf("Credits: Uses XcodeEditor (https://github.com/jasperblues/XcodeEditor)\n\n");
  printf("%s lists source code files from the build target of an Xcode project.\n\n", PROGRAM_NAME);
  printf("usage: %s [vh] -p <xcodeproj path> -t <target name> [--root-path <relative-root-path>]\n", PROGRAM_NAME);
  printf("  -p, --project      path to .xcodeproj file\n");
  printf("  -t, --target       name of build target in project\n");
  printf("  -r, --root-path    relative root path for output files\n");
  printf("                     (by default output is realtive to project path)\n");
  printf("  -h, --help         print this message\n\n");
  printf("example:\n");
  printf("%s -p ~/repos/drive-basestation/projects/iOS/BaseStation.xcodeproj -t BaseStation -r ~/repos/drive-basestation/source/anki\n", PROGRAM_NAME);
}

int parse_args(int argc, char * const argv[])
{
  int opt = 0;
  int long_index =0;
  while ((opt = getopt_long(argc, argv, "hp:t:r:",
                            Options, &long_index )) != -1) {
    switch (opt) {
      case 'h' : do_help = 1;
        break;
      case 'p' :
        bzero(project, MAXPATHLEN);
        memcpy(project, optarg, strlen(optarg));
        break;
      case 't' :
        bzero(target, MAXPATHLEN);
        memcpy(target, optarg, strlen(optarg));
        break;
      case 'r' :
        bzero(relative_path, MAXPATHLEN);
        memcpy(relative_path, optarg, strlen(optarg));
        break;
      default:
        return EXIT_FAILURE;
    }
  }
  
  return EXIT_SUCCESS;
}

int main(int argc, char * const argv[])
{
@autoreleasepool {
  if (do_help || parse_args(argc, argv) == EXIT_FAILURE) {
    print_usage();
    exit(EXIT_FAILURE);
  }
  
  if (strlen(project) == 0 || strlen(target) == 0) {
    print_usage();
    exit(EXIT_FAILURE);
  }
  
  NSString *projectPath = [NSString stringWithUTF8String:project];
  NSString *targetName = [NSString stringWithUTF8String:target];
  NSString *relativePath = (strlen(relative_path) > 0) ? [NSString stringWithUTF8String:relative_path] : nil;
  relativePath = [relativePath stringByStandardizingPath];
  
  NSFileManager *fm = [NSFileManager defaultManager];
  BOOL isDir = NO;
  if (![fm fileExistsAtPath:projectPath] && isDir) {
    fprintf(stderr, "[ERROR]: Project file not found: %s\n", projectPath.UTF8String);
    exit(EXIT_FAILURE);
  }
  
  XCProject *project = [XCProject projectWithFilePath:projectPath];
  if (!project) {
    fprintf(stderr, "[ERROR]: Error loading project\n");
    exit(EXIT_FAILURE);
  }
  
  XCTarget *target = [project targetWithName:targetName];
  if (!target) {
    fprintf(stderr, "[ERROR]: Target not found: %s\n", targetName.UTF8String);
    exit(EXIT_FAILURE);
  }
  
  NSArray *members = [target members];
  
  NSString *projectRootDir = [[projectPath stringByDeletingLastPathComponent] stringByStandardizingPath];
  for (XCSourceFile *srcFile in members) {
    if (srcFile.type == SourceCodeCPlusPlus || /* .cpp */
        srcFile.type == SourceCodeC || /* .m */
        srcFile.type == SourceCodeObjC || /* .m */
        srcFile.type == SourceCodeObjCPlusPlus || /* .mm */
        srcFile.type == SourceCodeHeader /* .h */) {
      NSString *projectRelativePath = [[srcFile pathRelativeToProjectRoot] stringByStandardizingPath];
      NSString *fullPath = [[projectRootDir stringByAppendingPathComponent:projectRelativePath] stringByStandardizingPath];
      
      NSString *path = fullPath;
      if (relativePath) {
        NSRange rpRange = [fullPath rangeOfString:relativePath];
        if (rpRange.location != NSNotFound) {
          NSUInteger startIndex = (rpRange.location + rpRange.length + 1);
          if (startIndex < fullPath.length) {
            path = [fullPath substringFromIndex:(rpRange.location + rpRange.length + 1)];
          }
        } else {
          fprintf(stderr, "root-path: %s not found in %s", relativePath.UTF8String, path.UTF8String);
          exit(EXIT_FAILURE);
        }
      }

      printf("%s\n", path.UTF8String);
    }
  }
  
  // For headers, we would likely need to iterate over all files
  // in the project: [project files]

}
  return EXIT_SUCCESS;
}

