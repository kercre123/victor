//
//  NSString+URLEncoding.m
//  DAS
//
//  Created by Mark Pauley on 3/5/13.
//  Copyright (c) 2013 Anki. All rights reserved.
//

#import "NSString+URLEncoding.h"
#import <CoreFoundation/CoreFoundation.h>

@implementation NSString_URLEncoding
+(NSString *)urlEncodeString:(NSString*)string usingEncoding:(NSStringEncoding)encoding {
  NSCharacterSet* urlSafeCharacterSet = [NSCharacterSet characterSetWithCharactersInString:@"!*'\"();:@&=+$,/?%#[]% "].invertedSet;
  return [string stringByAddingPercentEncodingWithAllowedCharacters: urlSafeCharacterSet];
}
@end
