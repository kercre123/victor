//
//  AppDelegate.h
//  mDNSListener
//
//  Created by Paul Aluri on 1/29/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreData/CoreData.h>

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;

@property (readonly, strong) NSPersistentContainer *persistentContainer;

- (void)saveContext;


@end

