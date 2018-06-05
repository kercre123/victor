//
//  main.m
//  mac-client
//
//  Created by Paul Aluri on 2/28/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "argparse.hpp"
#import "BleCentral.h"
#import "SyncController.h"
#import "Requests.h"

int main(int argc, const char * argv[]) {
  @autoreleasepool {
    // insert code here...
    ArgumentParser parser;
    BleCentral* central = [[BleCentral alloc] init];
    
    parser.addArgument("-f", "--filter", 1, true);
    parser.addArgument("-c", "--clean", 0, true);
    parser.addArgument("-v", "--verbose", 0, true);
    parser.addArgument("-h", "--help", 0, true);
    parser.addArgument("-s", "--sync", 1, true);
    parser.addArgument("-d", "--download", 0, true);
    parser.addArgument("-u", "--ota-update", 1, true);
    parser.addArgument("-m", "--ota-update-ap", 1, true);
    parser.addArgument("-l", "--local", 0, true);
    
    try {
      parser.parse(argc, argv);
    } catch(int error) {
      printf("Error parsing command line args.\n");
    }
    
    if(parser.exists("--help")) {
      printf("%s\n", parser.usage().c_str());
      printf("\n");
      [central printHelp];
      return 0;
    }
    
    [central setVerbose:parser.exists("--verbose")];
    [central setDownload:parser.exists("--download")];
    
    bool sync = parser.exists("--sync");
    bool otaupdate = parser.exists("--ota-update");
    bool otaupdateap = parser.exists("--ota-update-ap");
    bool useLocal = parser.exists("--local");
    bool noninteractive = otaupdate || otaupdateap || sync;
    
    if(noninteractive) {
      std::string c = "";
      std::string arg = "";
      
      if(sync) {
        c = parser.retrieve<std::string>("sync");
      } else if(otaupdate) {
        c = "ota-update";
        arg = parser.retrieve<std::string>(c);
      } else if(otaupdateap) {
        c = "ota-update-ap";
        arg = parser.retrieve<std::string>(c);
      }
      
      Requests* requests = [[Requests alloc] initWithCentral:central];
      SyncController* syncController = [[SyncController alloc] init];
      syncController.useLocal = useLocal;
      [syncController setCommand:c];
      [syncController setArg:arg];
      central.delegate = requests;
      central.syncdelegate = syncController;
      
      syncController.requests = requests;
    }
    
    std::string s;
    if(parser.exists("--filter")) {
      s = parser.retrieve<std::string>("filter");
    }
    
    if(parser.exists("--clean")) {
      parser.retrieve<std::string>("clean");
      [central resetDefaults];
    }
    
    NSString* name = [NSString stringWithUTF8String:s.c_str()];
    [central StartScanning:name];
    
    dispatch_main();
  }
  return 0;
}
