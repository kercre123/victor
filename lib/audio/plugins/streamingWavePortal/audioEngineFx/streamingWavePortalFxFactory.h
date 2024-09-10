/*
 * File: streamingWavePortalFxFactory.h
 *
 * Author: Jordan Rivas
 * Created: 07/06/18
 *
 * Description: StreamingWavePortal Plug-in common header to hook the plug-in into Wwise’s plug-in create & set
 *              parameters function pointers.
 *
 * Copyright: Anki, Inc. 2018
 */

#ifndef __AnkiAudio_PlugIns_StreamingWavePortalFxFactory_H__
#define __AnkiAudio_PlugIns_StreamingWavePortalFxFactory_H__

#ifndef EXCLUDE_ANKI_AUDIO_LIBS

#include "AK/SoundEngine/Common/IAkPlugin.h"

#if !defined AK_3DS && !defined AK_VITA_HW
	AK_STATIC_LINK_PLUGIN(StreamingWavePortal)
#endif


#endif // EXCLUDE_ANKI_AUDIO_LIBS
#endif /* __AnkiAudio_PlugIns_StreamingWavePortalFxFactory_H__ */
