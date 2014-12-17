//
//  NSUserDefaults+UI.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/15/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import "NSUserDefaults+UI.h"

@implementation NSUserDefaults (UI)

// Thank you Pauley!!
#pragma mark - Macros
#define RHUserDefaultDefinePropertyObjDefault(PROP_TYPE, PROP_NAME_GETTER, PROP_NAME_SETTER, PROP_KEY, PROP_DEFAULT) \
+(PROP_TYPE *)PROP_NAME_GETTER { \
PROP_TYPE *property = (PROP_TYPE *)[[NSUserDefaults standardUserDefaults] objectForKey: PROP_KEY ]; \
if(nil == property) { return PROP_DEFAULT; }\
return property; \
} \
+ (void)PROP_NAME_SETTER :(PROP_TYPE *)property {\
if(nil == property) {\
[[NSUserDefaults standardUserDefaults] removeObjectForKey: PROP_KEY ]; \
} \
else {\
[[NSUserDefaults standardUserDefaults] setObject:property forKey: PROP_KEY ]; \
}\
}

#define RHUserDefaultDefinePropertyObj(PROP_TYPE, PROP_NAME_GETTER, PROP_NAME_SETTER, PROP_KEY)\
RHUserDefaultDefinePropertyObjDefault(PROP_TYPE, PROP_NAME_GETTER, PROP_NAME_SETTER, PROP_KEY, nil)

#define RHUserDefaultDefinePropertyFloat(PROP_NAME_GETTER, PROP_NAME_SETTER, PROP_KEY, PROP_DEFAULT) \
RHUserDefaultDefinePropertyObj(NSNumber, PROP_NAME_GETTER ## _NUM, PROP_NAME_SETTER ## _NUM, PROP_KEY) \
+(float)PROP_NAME_GETTER { \
NSNumber* numberVal = [self PROP_NAME_GETTER ## _NUM]; \
if(nil == numberVal) { \
return (PROP_DEFAULT); \
} \
return numberVal.floatValue; \
} \
+ (void)PROP_NAME_SETTER :(float)floatVal {\
[self PROP_NAME_SETTER ## _NUM : [NSNumber numberWithFloat:floatVal]];\
}

#define RHUserDefaultDefinePropertyInt(PROP_NAME_GETTER, PROP_NAME_SETTER, PROP_KEY, PROP_DEFAULT) \
RHUserDefaultDefinePropertyObj(NSNumber, PROP_NAME_GETTER ## _NUM, PROP_NAME_SETTER ## _NUM, PROP_KEY) \
+(int)PROP_NAME_GETTER { \
NSNumber* numberVal = [self PROP_NAME_GETTER ## _NUM]; \
if(nil == numberVal) { \
return (PROP_DEFAULT); \
} \
return numberVal.intValue; \
} \
+ (void)PROP_NAME_SETTER :(int)intVal {\
[self PROP_NAME_SETTER ## _NUM : [NSNumber numberWithInt:intVal]];\
}

#define RHUserDefaultDefinePropertyUnsignedInt(PROP_NAME_GETTER, PROP_NAME_SETTER, PROP_KEY, PROP_DEFAULT) \
RHUserDefaultDefinePropertyObj(NSNumber, PROP_NAME_GETTER ## _NUM, PROP_NAME_SETTER ## _NUM, PROP_KEY) \
+(NSUInteger)PROP_NAME_GETTER { \
NSNumber* numberVal = [self PROP_NAME_GETTER ## _NUM]; \
if(nil == numberVal) { \
return (PROP_DEFAULT); \
} \
return numberVal.unsignedIntegerValue; \
} \
+ (void)PROP_NAME_SETTER :(NSUInteger)uIntVal { \
[self PROP_NAME_SETTER ## _NUM : [NSNumber numberWithUnsignedInteger:uIntVal]]; \
}

#define RHUserDefaultDefinePropertyBool(PROP_NAME_GETTER, PROP_NAME_SETTER, PROP_KEY, PROP_DEFAULT) \
RHUserDefaultDefinePropertyObj(NSNumber, PROP_NAME_GETTER ## _NUM, PROP_NAME_SETTER ## _NUM, PROP_KEY) \
+(BOOL)PROP_NAME_GETTER { \
NSNumber* numberVal = [self PROP_NAME_GETTER ## _NUM]; \
if(nil == numberVal) { \
return (PROP_DEFAULT); \
} \
return numberVal.boolValue; \
} \
+ (void)PROP_NAME_SETTER :(BOOL)boolVal {\
[self PROP_NAME_SETTER ## _NUM : [NSNumber numberWithBool:boolVal]];\
}


#pragma mark - User Default Definitions

NSString* const DefaultKeyLastHostAdvertisingIP = @"lastHostAdvertisingIP";
RHUserDefaultDefinePropertyObj(NSString, lastHostAdvertisingIP, setLastHostAdvertisingIP, DefaultKeyLastHostAdvertisingIP)

NSString* const DefaultKeyLastBasestationIP = @"lastBasestationIP";
RHUserDefaultDefinePropertyObj(NSString, lastBasestationIP, setLastBasestationIP, DefaultKeyLastBasestationIP)

NSString* const DefaultKeyAutoConnectRobot = @"autoConnectRobot";
RHUserDefaultDefinePropertyBool(autoConnectRobot, setAutoConnectRobot, DefaultKeyAutoConnectRobot, YES)
//
//+ (BOOL)autoConnectRobot;
//+ (void)setAutoConnectRobot:(BOOL)autoConnect;




@end
