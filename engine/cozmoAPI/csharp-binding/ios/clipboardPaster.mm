//
//  clipboardPaster.mm
//  cozmoGame
//
//  Created by Pohung Chen on 3/25/16.
//
//

#import <Foundation/Foundation.h>
#import "clipboardPaster.h"
#import <UIKit/UIKit.h>

void WriteToClipboard(const char* log) {
  [UIPasteboard generalPasteboard].string = [NSString stringWithUTF8String:log];
  [UIPasteboard generalPasteboard].persistent = YES;
}
