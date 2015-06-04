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

// http://answers.unity3d.com/questions/191234/unity-ios-function-pointers.html
[AttributeUsage(AttributeTargets.Method)]
public class MonoPInvokeCallbackAttribute : System.Attribute
{
	private Type type;
	public MonoPInvokeCallbackAttribute( Type t ) { type = t; }
}

public static class CozmoBinding {

	//[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void LogCallback(int logLevel, [MarshalAs(UnmanagedType.LPStr)] string message);

	[DllImport ("__Internal")]
	public static extern bool cozmo_set_log_callback(LogCallback callback, int min_log_level);

	[DllImport("__Internal")]
	public static extern int cozmo_startup (string configuration_data);

	[DllImport("__Internal")]
	public static extern int cozmo_shutdown();
	
	[DllImport("__Internal")]
	public static extern int cozmo_update(float current_time);
}

