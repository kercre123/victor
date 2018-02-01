//
//  main.m
//  sodium-video
//
//  Created by Paul Aluri on 1/29/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "createFile.h"

int main(int argc, const char * argv[]) {
  @autoreleasepool {
      // insert code here...
      NSLog(@"Hello, World!");
    
    SodiumVideo* sv = new SodiumVideo();
    
    delete sv;
  }
  return 0;
}
