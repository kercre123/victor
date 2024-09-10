//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2017 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////


#ifndef _AKAUTOHELPER_H_
#define _AKAUTOHELPER_H_

//ConvertUTF16ToChar
static AKRESULT ConvertUTF16ToChar(AkOSChar *in_pDest, AkUInt8 * in_pSrc, AkUInt32 in_uDestMaxSize, AkUInt32 in_uSrcLength)
{
	// WARNING!!! This function will only work for English language ASCII characters (as the second byte of UTF16 will always be zero in this case only).
	// Special characters or characters from other languages may not convert to the expected results.
	AkUInt32 uUtf16NbBytes = sizeof(AkUtf16);

	if(((in_uSrcLength+1) > (in_uDestMaxSize)))
	{
		return AK_Fail;
	}

	//Convert UFT16 to char
	for(AkUInt32 i=0; i<in_uSrcLength; i++)
	{
		in_pDest[i] = (AkOSChar)in_pSrc[i*uUtf16NbBytes];
	}
	//Insert null terminated char
	in_pDest[in_uSrcLength] = '\0';

	return AK_Success;
}

#endif //_AKAUTOHELPER_H_
