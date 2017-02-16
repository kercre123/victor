//
//  NSString+URLEncoding.h
//  DAS
//
//  Created by Mark Pauley on 3/5/13.
//  Copyright (c) 2013 Anki. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSString_URLEncoding : NSObject
+(NSString *)urlEncodeString:(NSString*)string usingEncoding:(NSStringEncoding)encoding;
@end
