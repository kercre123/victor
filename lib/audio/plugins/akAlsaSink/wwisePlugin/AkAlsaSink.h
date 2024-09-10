//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2016 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

class CAkAlsaSinkApp : public CWinApp
{
public:
	CAkAlsaSinkApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
