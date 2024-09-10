#pragma once

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"        // main symbols


// CVocoderApp
class CVocoderApp : public CWinApp
{
public:
    CVocoderApp();

// Overrides
public:
    virtual BOOL InitInstance();

    DECLARE_MESSAGE_MAP()
};
