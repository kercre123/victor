//
//  AppDelegate.h
//  ios-client
//
//  Created by Paul Aluri on 3/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreData/CoreData.h>

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;

@property (readonly, strong) NSPersistentContainer *persistentContainer;

- (void)saveContext;


@end

