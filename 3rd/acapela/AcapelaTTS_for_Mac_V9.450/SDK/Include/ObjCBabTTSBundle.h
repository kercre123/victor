/* ObjCBabTTSBundle.h
   Objective-C/Objective-C++ bundling facility class header file.
   
   Copyright (c) 2005-2010 Acapela Group. All rights reserved. */

/* To bundle your Objective-C/Objective-C++ application:
   1. Include this header file
   2. Load the AcaTTS dynamic library, for example:
      
      BabTtsInitDll();
      
   3. Use the following code snippet and replace 
      const char* myLic="#REVT...#"; with the line extracted from the license 
      file (a file with the ".lic.h" extension) sent to you by your sales 
      contact. For additional information, refer to the AcaMul-Bundling.pdf 
      documentation file.
	   
	   const char* myLic="#REVT...#"; 
	   BabTTSBundle *ttsBundle = [[BabTTSBundle alloc] initWithBundleID:20];
	   [ttsBundle setLicense:myLic];
 */

#import <Foundation/Foundation.h>
#include "stdio.h"
#include "dlfcn.h"

#if defined(__APPLE__)
	#define OBJCBABTTSBUNDLE_ACATTS_LIB_PREFIX "./"
	#define OBJCBABTTSBUNDLE_ACATTS_LIB_NAKEDNAME "libacatts.dylib"
#else 
	#error This file is designed to be used on Mac OS X only using Objective-C/Objective-C++.
#endif

#if !defined(BABTTS_LIB)
	#define BABTTS_LIB \
		OBJCBABTTSBUNDLE_ACATTS_LIB_PREFIX OBJCBABTTSBUNDLE_ACATTS_LIB_NAKEDNAME
#endif
	
#if !defined(BABTTS_LIBNAME)
	#define BABTTS_LIBNAME \
		OBJCBABTTSBUNDLE_ACATTS_LIB_NAKEDNAME
#endif

@interface BabTTSBundle : NSObject {
	@public
		long int bundleID;
	@protected
		NSString *libraryPath;
}

-(id)init;
-(id)initWithBundleID:(long int)ID;
-(void)dealloc;

-(BOOL)setLicense:(const char*) lpszLicense;

-(void)setLibraryPath:(NSString *) path;
@end

typedef int (*pPrivBabTTS_SetLicense)(const char *lpszLic);
typedef struct
{
	short garbage;
	unsigned long dwProductConstant;
} PrivateData_t;

static void *BabTtsBundleDLL = NULL;
static pPrivBabTTS_SetLicense PrivBabTTS_SetLicense = NULL;

@implementation BabTTSBundle

-(id)init
{
	self = [super init];
	return self;
}

-(id)initWithBundleID:(long int)bundleID
{
	char szTemp[512];
	Dl_info infoDylib;
	const char *libraryPathStr = 
		[libraryPath cStringUsingEncoding:NSMacOSRomanStringEncoding]
	;
	
	if ((NULL == libraryPathStr) || ('\0' == libraryPathStr[0]))
	{
		if (NULL != BabTTS_Open)
		{
			if (dladdr((void *) BabTTS_Open, &infoDylib))
			{
				snprintf(szTemp, sizeof(szTemp), "%s", infoDylib.dli_fname);
			}
		}
		else
		{
			if (dladdr("", &infoDylib))
			{
				char *lastseparator;
				snprintf(szTemp, sizeof(szTemp), "%s", infoDylib.dli_fname);
				lastseparator = strrchr(szTemp, '/');
				szTemp[lastseparator-szTemp + 1] = '\0';
				strcat(szTemp, BABTTS_LIB);
			}
			/* Last resort. */
			else
			{
				snprintf(szTemp, sizeof(szTemp), "%s", BABTTS_LIB);
			}
		}
	}
	else
	{
		snprintf(szTemp, sizeof(szTemp), "%s", libraryPathStr);
	}
	
	BabTtsBundleDLL = dlopen(szTemp, RTLD_NOW);
	
	if (NULL != BabTtsBundleDLL)
	{		
		PrivBabTTS_SetLicense = (pPrivBabTTS_SetLicense) dlsym(
			BabTtsBundleDLL, (const char*) "PrivBabTTS_SetLicense"
		);
		
		if (NULL != PrivBabTTS_SetLicense)
		{
			NSLog(@"LoadSymbol of BabTtsBundleDLL OK");
			return self;
		}
		else 
		{
			NSLog(
				@"Wrong version of AcaTTS: PrivBabTTS_SetLicense (%p)", 
				PrivBabTTS_SetLicense
			);
			dlclose(BabTtsBundleDLL);
			return nil;
		}
	}
	else
		return nil;
}

-(BOOL)registerWithClientKey:(unsigned long) dwClientKey andPasswd:(unsigned long) dwPassword
{
	/* Returned for backward compatibility. */
	return YES;
}

-(void)dealloc
{
	if (NULL != BabTtsBundleDLL)
	{
		dlclose(BabTtsBundleDLL);
		BabTtsBundleDLL = NULL;
	}
	
	[super dealloc];
}

-(BOOL)setLicense:(const char*) lpszLicense;
{
	int ret=PrivBabTTS_SetLicense(lpszLicense);
	if (ret)
		return true;
	else
		return false;
}

-(void)setLibraryPath:(NSString *) path;
{
	if (libraryPath != path)
	{
		[libraryPath release];
		libraryPath = [path retain];
	}
}

@end
