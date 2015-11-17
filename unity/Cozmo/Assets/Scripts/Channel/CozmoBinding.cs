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

  [DllImport("__Internal")]
  private static extern int cozmo_startup(string configuration_data);

  [DllImport("__Internal")]
  private static extern int cozmo_shutdown();

  [DllImport("__Internal")]
  private static extern int cozmo_update(float current_time);

  public static void Startup(string configurationData) {
    if (initialized) {
      sDAS.Warn("Reinitializing because Startup was called twice...");
      Shutdown();
    }

    AnkiResult result = AnkiResult.RESULT_OK;
#if !UNITY_EDITOR
    Profiler.BeginSample ("CozmoBinding.cozmo_startup");
    result = (AnkiResult)CozmoBinding.cozmo_startup (configurationData);
    Profiler.EndSample ();
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
#if !UNITY_EDITOR
      Profiler.BeginSample("CozmoBinding.cozmo_shutdown");
      result = (AnkiResult)CozmoBinding.cozmo_shutdown();
      Profiler.EndSample();
#endif

      if (result != AnkiResult.RESULT_OK) {
        sDAS.Error("CozmoBinding.Shutdown [cozmo_shutdown]: error code " + result.ToString());
      }
    }
  }

  public static void Update(float frameTime) {
    if (initialized) {
      AnkiResult result = AnkiResult.RESULT_OK;
#if !UNITY_EDITOR
      Profiler.BeginSample("CozmoBinding.cozmo_update");
      result = (AnkiResult)CozmoBinding.cozmo_update (frameTime);
      Profiler.EndSample();
#endif
      if (result != AnkiResult.RESULT_OK) {
        sDAS.Error("CozmoBinding.Update [cozmo_update]: error code " + result.ToString());
      }
    }
  }
}

