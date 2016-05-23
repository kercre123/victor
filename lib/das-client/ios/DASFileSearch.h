//
//  DASFileSearch.h
//  DAS
//
//  Created by Mark Pauley on 9/27/12.
//  Copyright (c) 2012 Anki. All rights reserved.
//

#import <Foundation/Foundation.h>

// can this be called more than once?
typedef void (^DASFileSearchCompletionBlock)(NSArray* searchResults, NSError* error);

@interface DASFileSearch : NSOperation

-(id)initWithRegularExpression:(NSString*)searchString;

@property (nonatomic, strong) DASFileSearchCompletionBlock searchCompletionBlock;
@property (nonatomic, readonly) NSArray* searchResults;

@end
