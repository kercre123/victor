//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// The main automotive audio engine interface.

extern AKRESULT RegisterMasterSink(void *pMasterSink);
extern AKRESULT UnRegisterMasterSink();
extern AKRESULT MasterSignalThread();

