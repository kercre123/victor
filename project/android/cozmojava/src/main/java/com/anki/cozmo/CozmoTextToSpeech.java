package com.anki.cozmo;

import android.content.Context;
import com.anki.daslib.DAS;
import com.acapelagroup.android.tts.acattsandroid;
import com.acapelagroup.android.tts.acattsandroid.iTTSEventsCallback;
import com.acapelagroup.android.tts.acattsandroid.iTTSSamplesCallback;

/**
 * CozmoTextToSpeech
 */
public class CozmoTextToSpeech implements iTTSEventsCallback, iTTSSamplesCallback {

  // Required to load native TTS library on startup
  static {
    System.loadLibrary("DAS");
    System.loadLibrary("acattsandroid");
  }

  // Declarations from anki/common/types.h
  private static final int RESULT_OK = 0x00;
  private static final int RESULT_FAIL = 0x01;
  private static final int RESULT_FAIL_IO = 0x02000000;
  private static final int RESULT_FAIL_INVALID_PARAMETER = 0x03000000;

  // Log helpers
  // JNI DAS does not support channels, so prefix all events with magic tag
  private static final String TAG = "TextToSpeech.";

  private static void error(String evt, String text) {
    DAS.Error(TAG+evt, text);
  }

  private static void warn(String evt, String text) {
    DAS.Warn(TAG+evt, text);
  }

  private static void info(String evt, String text) {
    DAS.Info(TAG+evt, text);
  }

  private static void debug(String evt, String text) {
    DAS.Debug(TAG+evt, text);
  }

  private static void trace(String evt, String text) {
    // Enable for verbose debug messages
    // DAS.Debug(TAG+evt, text);
  }

  // Fixed parameters
  private static final int TTS_LEADINGSILENCE_MS = 50;  // Minimum allowed by Acapela TTS SDK
  private static final int TTS_TRAILINGSILENCE_MS = 50; // Minimum allowed by Acapela TTS SDK

  //
  // JNI declaration of C++ function to be invoked as callback.
  // Note that method name and signature must match definition in TextToSpeechProvider_android.cpp
  //
  private static native void callback(short[] samples, long count);

  // Global singleton
  private static CozmoTextToSpeech sInstance = null;

  // Acapela TTS SDK interface
  private acattsandroid _tts = null;

  // Running count of samples
  private int _count = 0;

  // Class constructor
  public CozmoTextToSpeech(android.content.Context ctx) {
    final String EVT = "CozmoTextToSpeech.init";

    debug(EVT, "ctx="+ctx);

    // Initialize SDK object with callbacks
    _tts = new acattsandroid(ctx, this, this);

    // Enable SDK log output
    // _tts.setLog(true);

    String version = _tts.getVersion();
    debug(EVT, "TTS SDK version=" + version);

  }

  // Class destructor
  public void finalize() {
    final String EVT = "CozmoTextToSpeech.finalize";
    debug(EVT, "Finalize");
    if (_tts != null) {
      _tts.shutdown();
      _tts = null;
    }
  }

  // TTS Events callback
  @Override
  public void ttsevents(long type, long param1, long param2, long param3, long param4) {
    final String EVT = "CozmoTextToSpeech.ttsevents";
    if (type == acattsandroid.EVENT_TEXT_START) {
      trace(EVT, "Text " + param1 + " start");
    } else if (type == acattsandroid.EVENT_TEXT_END) {
      trace(EVT, "Text " + param1 + " end");
    } else if (type == acattsandroid.EVENT_AUDIO_END) {
      trace(EVT, "Audio end");
    } else if (type == acattsandroid.EVENT_WORD_POS) {
      trace(EVT, "Event word pos");
    } else if (type == acattsandroid.EVENT_VISEME_POS) {
      trace(EVT, "Phoneme " + param1);
    }
  }

  // TTS Samples callback
  @Override
  public void samples(short[] data, long count) {
    final String EVT = "CozmoTextToSpeech.samples";
    trace(EVT, "count=" + count);
    _count += count;
    callback(data, count);
  }

  // Load given voice
  public int doLoadVoice(String path, String voice, int userid, int password, String license) {
    final String EVT = "CozmoTextToSpeech.doLoadVoice";

    debug(EVT, "path=" + path + " voice=" + voice);

    // Enumerate voices available with given resource path
    String[] voiceDirPaths = {path};
    String[] voicesList = _tts.getVoicesList(voiceDirPaths);
    if (null == voicesList) {
      error(EVT, "Unable to get voices");
      return RESULT_FAIL_INVALID_PARAMETER;
    }

    for (String s : voicesList) {
      debug(EVT, "Available voice=" + s);
    }

    int err = _tts.setLicense(userid, password, license);
    if (0 != err) {
      error(EVT, "Unable to set license, err=" + err);
      return RESULT_FAIL_INVALID_PARAMETER;
    }
    // Attempt to load requested voice
    err = _tts.load(voice, "");
    if (0 != err) {
      error(EVT, "Unable to load voice, err="+err);
      return RESULT_FAIL_INVALID_PARAMETER;
    }

    err = _tts.setTTSSettings("LEADINGSILENCE", TTS_LEADINGSILENCE_MS);
    if (0 != err) {
      error(EVT, "Unable to set leading silence, err="+err);
    }

    err = _tts.setTTSSettings("TRAILINGSILENCE", TTS_TRAILINGSILENCE_MS);
    if (0 != err) {
      error(EVT, "Unable to set trailing silence, err="+err);
    }

    return RESULT_OK;
  }

  // Create audio for given text
  public int doCreateAudioData(String text, int speed, int shaping, int pitch) {
    final String EVT = "CozmoTextToSpeech.doCreateAudioData";
    debug(EVT, "text=" + text + " speed=" + speed + " shaping=" + shaping + " pitch=" + pitch);
    int err = _tts.setSpeechRate(speed);
    if (0 != err) {
      error(EVT, "Unable to set speech rate, err="+err);
      return RESULT_FAIL_INVALID_PARAMETER;
    }

    err = _tts.setTTSSettings("VOICESHAPE", shaping);
    if (0 != err) {
      error(EVT, "Unable to set shaping, err="+err);
      return RESULT_FAIL_INVALID_PARAMETER;
    }

    err = _tts.setPitch(pitch);
    if (0 != err) {
      error(EVT, "Unable to set pitch, err="+err);
      return RESULT_FAIL_INVALID_PARAMETER;
    }

    // Reset data count
    _count = 0;

    err = _tts.speak(text);
    if (0 != err) {
      error(EVT, "Unable to speak, err="+err);
      return RESULT_FAIL;
    }

    int result = RESULT_OK;
    // Wait for TTS to complete. Note there's no time limit on this loop...
    // We rely on TTS SDK to update status correctly.
    while (_tts.isSpeaking()) {
      try {
        Thread.sleep(100);
      } catch (java.lang.InterruptedException ex) {
        warn(EVT, "TTS was interrupted");
        result = RESULT_FAIL_IO;
        break;
      }
    }

    _tts.stop();

    // Report final count
    debug(EVT, "count=" + _count);

    return result;
  }

  // Singleton helpers
  public static void register(android.content.Context ctx) {
    sInstance = new CozmoTextToSpeech(ctx);
  }

  public static void unregister() {
    sInstance = null;
  }

  public static int loadVoice(String path, String voice, int userid, int password, String license) {
    return sInstance.doLoadVoice(path, voice, userid, password, license);
  }

  public static int createAudioData(String text, int speed, int shaping, int pitch) {
    return sInstance.doCreateAudioData(text, speed, shaping, pitch);
  }
}
