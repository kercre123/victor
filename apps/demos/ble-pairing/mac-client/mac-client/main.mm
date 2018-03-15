//
//  main.m
//  mac-client
//
//  Created by Paul Aluri on 2/28/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "BleCentral.h"

int main(int argc, const char * argv[]) {
  @autoreleasepool {
    // insert code here...
    BleCentral* central = [[BleCentral alloc] init];
    
//    char pin[100];
//    scanf("%100s",pin);
//    printf("Another log....\n");
//    fflush(stdout);
//    
//    printf("READ: %s\n", pin);
    
    [central StartScanning];
    dispatch_main();
  }
  return 0;
}
