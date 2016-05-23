//
//  DASFileSearch.m
//  DAS
//
//  Created by Mark Pauley on 9/27/12.
//  Copyright (c) 2012 Anki. All rights reserved.
//

#import "DASFileSearch.h"
#import <AFNetworking/AFNetworking.h>

#define DASFileURL "http://gls.anki.com"
#define DASSearchPath "/api/findgame"

dispatch_once_t searchClientOnceToken;

AFHTTPRequestOperationManager* SearchClient = nil;

static void initClient() {
  SearchClient = [[AFHTTPRequestOperationManager alloc] initWithBaseURL:[NSURL URLWithString:@DASFileURL]];
}

@implementation DASFileSearch {
  AFHTTPRequestOperationManager* _searchClient;
  AFHTTPRequestOperation* _requestOperation;
  NSString* _searchString;
  BOOL _isFinished;
}

@synthesize searchCompletionBlock = _searchCompletionBlock;
@synthesize searchResults = _searchResults;

-(void)setClient:(AFHTTPRequestOperationManager*)client {
  _searchClient = client;
}

-(id)initWithRegularExpression:(NSString *)searchString {
  if((self = [super init])) {
    _searchString = searchString;
  }
  return self;
}

-(AFHTTPRequestOperationManager*)searchClient {
  if(!_searchClient) {
    dispatch_once(&searchClientOnceToken, ^{
      initClient();
    });
    _searchClient = SearchClient;
  }
  return _searchClient;
}

-(BOOL)isFinished {
  return _isFinished;
}

-(void)main {
  
  _requestOperation = [[self searchClient] GET:@DASSearchPath parameters:@{@"search": _searchString} success:^(AFHTTPRequestOperation *operation, id responseObject) {
    [self willChangeValueForKey:@"isFinished"];
    if(self.searchCompletionBlock) {
      NSDictionary* dict = responseObject;
      NSArray* games = dict[@"games"];
      // what if games is empty or the response is malformed? (pauley)
      self.searchCompletionBlock(games, nil);
    }
    _isFinished = YES;
    [self didChangeValueForKey:@"isFinished"];
  } failure:^(AFHTTPRequestOperation *operation, NSError *error) {
    NSLog(@"Failure: %@", error);
    [self willChangeValueForKey:@"isFinished"];
    if(self.completionBlock) {
      self.searchCompletionBlock(nil, error);
    }
    _isFinished = YES;
    [self didChangeValueForKey:@"isFinished"];
  }];
}

-(void)cancel {
  [_requestOperation cancel];
  [super cancel];
}

@end
