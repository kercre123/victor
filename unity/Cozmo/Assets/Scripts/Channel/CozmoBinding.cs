using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

/// <summary>
/// Result codes that can be returned from C functions.
/// </summary>
public enum AnkiResult {
  RESULT_OK = 0,
  RESULT_FAIL = 0x00000001,
  RESULT_FAIL_MEMORY = 0x01000000,
  RESULT_FAIL_OUT_OF_MEMORY = 0x01000001,
  RESULT_FAIL_UNINITIALIZED_MEMORY = 0x01000002,
  RESULT_FAIL_ALIASED_MEMORY = 0x01000003,
  RESULT_FAIL_IO = 0x02000000,
  RESULT_FAIL_IO_TIMEOUT = 0x02000001,
  RESULT_FAIL_IO_CONNECTION_CLOSED = 0x02000002,
  RESULT_FAIL_INVALID_PARAMETER = 0x03000000,
  RESULT_FAIL_INVALID_OBJECT = 0x04000000,
  RESULT_FAIL_INVALID_SIZE = 0x05000000,
}

public static class CozmoBinding {

  private static readonly IDAS sDAS = DAS.GetInstance(typeof(CozmoBinding));

  private static bool initialized = false;

  public static Guid AppRunId = Guid.NewGuid();

  #if UNITY_IOS || UNITY_STANDALONE

  [DllImport("__Internal")]
  private static extern int cozmo_startup(string configuration_data);

  [DllImport("__Internal")]
  private static extern int cozmo_shutdown();

  [DllImport("__Internal")]
  private static extern int cozmo_wifi_setup(string wifiSSID, string wifiPasskey);

  [DllImport("__Internal")]
  private static extern void cozmo_send_to_clipboard(string logData);

  [DllImport("__Internal")]
  public static extern uint cozmo_transmit_engine_to_game([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1)] byte[] bytes, System.UIntPtr len);

  [DllImport("__Internal")]
  public static extern void cozmo_transmit_game_to_engine(byte[] bytes, System.UIntPtr len);

  [DllImport("__Internal")]
  public static extern uint cozmo_transmit_viz_to_game([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1)] byte[] bytes, System.UIntPtr len);

  [DllImport("__Internal")]
  public static extern void cozmo_execute_background_transfers();

  #elif UNITY_ANDROID

  [DllImport("cozmoEngine")]
  private static extern int cozmo_startup(string configuration_data);

  [DllImport("cozmoEngine")]
  private static extern int cozmo_shutdown();

  [DllImport("cozmoEngine")]
  private static extern int cozmo_wifi_setup(string wifiSSID, string wifiPasskey);

  [DllImport("cozmoEngine")]
  private static extern void cozmo_send_to_clipboard(string logData);

  [DllImport("cozmoEngine")]
  public static extern uint cozmo_transmit_engine_to_game([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1)] byte[] bytes, System.UIntPtr len);

  [DllImport("cozmoEngine")]
  public static extern void cozmo_transmit_game_to_engine(byte[] bytes, System.UIntPtr len);

  [DllImport("cozmoEngine")]
  public static extern uint cozmo_transmit_viz_to_game([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1)] byte[] bytes, System.UIntPtr len);

  [DllImport("cozmoEngine")]
  public static extern void cozmo_install_google_breakpad(string path);

  [DllImport("cozmoEngine")]
  public static extern void cozmo_uninstall_google_breakpad();

  [DllImport("cozmoEngine")]
  public static extern void cozmo_execute_background_transfers();

  #endif

  public static void Startup(JSONObject configurationData) {
    if (initialized) {
      sDAS.Warn("Reinitializing because Startup was called twice...");
      Shutdown();
    }

    configurationData.AddField("appRunId", AppRunId.ToString());

    AnkiResult result = AnkiResult.RESULT_OK;
    #if !UNITY_EDITOR && !UNITY_STANDALONE
    #if UNITY_ANDROID
    new AndroidJavaClass("java.lang.System").CallStatic("loadLibrary", "DAS");
    #endif
    Profiler.BeginSample ("CozmoBinding.cozmo_startup");
    result = (AnkiResult)CozmoBinding.cozmo_startup (configurationData.ToString());
    Profiler.EndSample ();
    #if UNITY_ANDROID
    new AndroidJavaClass("com.anki.cozmo.CozmoJava").CallStatic("init", GetCurrentActivity());
    #endif
    #endif
    
    if (result != AnkiResult.RESULT_OK) {
      sDAS.Error("CozmoBinding.Startup [cozmo_startup]: error code " + result.ToString());
    }
    else {
      initialized = true;
    }
  }

  public static void Shutdown() {
    if (initialized) {
      initialized = false;
      
      AnkiResult result = AnkiResult.RESULT_OK;
      #if !UNITY_EDITOR && !UNITY_STANDALONE
      Profiler.BeginSample("CozmoBinding.cozmo_shutdown");
      result = (AnkiResult)CozmoBinding.cozmo_shutdown();
      Profiler.EndSample();
      #endif

      if (result != AnkiResult.RESULT_OK) {
        sDAS.Error("CozmoBinding.Shutdown [cozmo_shutdown]: error code " + result.ToString());
      }
    }
  }

  public static void WifiSetup(string wifiSSID, string wifiPasskey) {
    if (initialized) {
      AnkiResult result = AnkiResult.RESULT_OK;
      #if !UNITY_EDITOR && !UNITY_STANDALONE
      Profiler.BeginSample("CozmoBinding.cozmo_wifi_setup");
      result = (AnkiResult)CozmoBinding.cozmo_wifi_setup(wifiSSID, wifiPasskey);
      Profiler.EndSample();
      #endif

      if (result != AnkiResult.RESULT_OK) {
        sDAS.Error("CozmoBinding.WifiSetup [cozmo_wifi_setup]: error code " + result.ToString());
      }
    }
  }

  public static void SendToClipboard(string log) {
    if (initialized) {
      #if !UNITY_EDITOR && !UNITY_STANDALONE
      cozmo_send_to_clipboard(log);
      #endif
    }

  }

  #if (UNITY_ANDROID && !UNITY_EDITOR)
  public static AndroidJavaObject GetCurrentActivity() {
    AndroidJavaClass unityPlayer = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
    return unityPlayer.GetStatic<AndroidJavaObject>("currentActivity");
  }
  #endif

}

