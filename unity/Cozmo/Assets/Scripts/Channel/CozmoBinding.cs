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
	RESULT_FAIL                      = 0x00000001,
	RESULT_FAIL_MEMORY               = 0x01000000,
	RESULT_FAIL_OUT_OF_MEMORY        = 0x01000001,
	RESULT_FAIL_UNINITIALIZED_MEMORY = 0x01000002,
	RESULT_FAIL_ALIASED_MEMORY       = 0x01000003,
	RESULT_FAIL_IO                   = 0x02000000,
	RESULT_FAIL_IO_TIMEOUT           = 0x02000001,
	RESULT_FAIL_IO_CONNECTION_CLOSED = 0x02000002,
	RESULT_FAIL_INVALID_PARAMETER    = 0x03000000,
	RESULT_FAIL_INVALID_OBJECT       = 0x04000000,
	RESULT_FAIL_INVALID_SIZE         = 0x05000000,
}

/// <summary>
/// Specifies to the compiler that the method could be called directly from native code.
/// </summary>
/// <remarks>
/// http://answers.unity3d.com/questions/191234/unity-ios-function-pointers.html
/// </remarks>
[AttributeUsage(AttributeTargets.Method)]
public class MonoPInvokeCallbackAttribute : System.Attribute
{
	private Type type;
	public MonoPInvokeCallbackAttribute( Type t ) { type = t; }
}

public static class CozmoBinding {

	//[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void LogCallback(int logLevel, [MarshalAs(UnmanagedType.LPStr)] string message);

	private static object syncLogs = new object();
	private static bool initialized = false;

	// Need to pin log callbacks in memory until we're sure they're no longer used.
	// We'll just keep adding to the list until there's a time we know we can clear some.
	private static int logCallbackHandlesUsed = 0;
	// Mutable structs need to be stored in arrays for consistency.
	private static GCHandle[] logCallbackHandles = new GCHandle[4];

	[DllImport ("__Internal")]
	private static extern int cozmo_set_log_callback(LogCallback callback, int min_log_level);

	[DllImport("__Internal")]
	private static extern int cozmo_startup (string configuration_data);

	[DllImport("__Internal")]
	private static extern int cozmo_shutdown();
	
	[DllImport("__Internal")]
	private static extern int cozmo_update(float current_time);

	public static void Startup(string configurationData)
	{
		if (initialized) {
			Debug.LogWarning ("Reinitializing because Startup was called twice...");
			Shutdown ();
		}

		AnkiResult result = AnkiResult.RESULT_OK;
#if !UNITY_EDITOR
		Profiler.BeginSample ("CozmoBinding.cozmo_startup");
		result = (AnkiResult)CozmoBinding.cozmo_startup (configurationData);
		Profiler.EndSample ();
#endif
		
		if (result != AnkiResult.RESULT_OK) {
			Debug.LogError ("CozmoBinding.Startup [cozmo_startup]: error code " + result.ToString());
		} else {
			initialized = true;
		}
	}

	public static void Shutdown()
	{
		if (initialized) {
			initialized = false;
			
			AnkiResult result = AnkiResult.RESULT_OK;
#if !UNITY_EDITOR
			Profiler.BeginSample("CozmoBinding.cozmo_shutdown");
			result = (AnkiResult)CozmoBinding.cozmo_shutdown();
			Profiler.EndSample();
#endif

			if (result != AnkiResult.RESULT_OK) {
				Debug.LogError ("CozmoBinding.Shutdown [cozmo_shutdown]: error code " + result.ToString());
			}
		}
	}

	public static void Update(float frameTime)
	{
		if (initialized) {
			AnkiResult result = AnkiResult.RESULT_OK;
#if !UNITY_EDITOR
			Profiler.BeginSample("CozmoBinding.cozmo_update");
			result = (AnkiResult)CozmoBinding.cozmo_update (frameTime);
			Profiler.EndSample();
#endif
			if (result != AnkiResult.RESULT_OK) {
				Debug.LogError ("CozmoBinding.Update [cozmo_update]: error code " + result.ToString());
			}
		}
	}

	/// <summary>
	/// Sets the log callback, making sure it is pinned in memory.
	/// </summary>
	/// <param name="logCallback">
	/// The log callback to call. It must be a method marked with MonoPInvokeCallbackAttribute.
	/// Do not use anonymous delegates. Do not call C++ code.
	/// </param>
	/// <param name="minLogLevel">The minimum log level to allow.</param>
	public static void SetLogCallback(LogCallback logCallback, int minLogLevel)
	{
		// Might be called on different threads.
		lock (syncLogs) {

			if (logCallbackHandlesUsed >= logCallbackHandles.Length) {
				GCHandle[] newHandles = new GCHandle[logCallbackHandles.Length * 2];
				Array.Copy (logCallbackHandles, newHandles, logCallbackHandlesUsed);
				logCallbackHandles = newHandles;
			}

#pragma warning disable 0219
			int previous_logCallbackHandledUsed = logCallbackHandlesUsed;
			if (logCallback != null) {
				if (!Attribute.IsDefined (logCallback.Method, typeof(MonoPInvokeCallbackAttribute))) {
					throw new ArgumentException ("You must mark a log callback with MonoPInvokeCallbackAttribute so that it works under AOT. Do not use anonymous delegates.", "logCallback");
				}
			
				// Need to pin the delegate in memory so that the garbage collector doesn't move it.
				// Technically we could lose the reference between the method return and array store, but
				// I believe that can only happen if a thread is killed.
				logCallbackHandlesUsed += 1;
				logCallbackHandles [previous_logCallbackHandledUsed] = GCHandle.Alloc (logCallback, GCHandleType.Pinned);
			}

			AnkiResult result = AnkiResult.RESULT_OK;
#if !UNITY_EDITOR
			Profiler.BeginSample ("CozmoBinding.cozmo_set_log_callback");
			result = (AnkiResult)cozmo_set_log_callback (logCallback, minLogLevel);
			Profiler.EndSample ();
#endif
		
			// if no error, then we know the only callback being used is the one we just gave it
			// this is the only case in which we can clean up
			if (result == AnkiResult.RESULT_OK) {
				for (int i = 0; i < previous_logCallbackHandledUsed; ++i) {
					if (logCallbackHandles [i].IsAllocated) {
						logCallbackHandles [i].Free ();
					}
				}
				logCallbackHandlesUsed -= previous_logCallbackHandledUsed;
			} else {
				Debug.LogError ("CozmoBinding.SetLogCallback [cozmo_set_log_callback]: error code " + result.ToString ());
			}
		}
	}
}

