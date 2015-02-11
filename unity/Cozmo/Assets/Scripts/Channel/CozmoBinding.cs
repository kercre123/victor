using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

/// <summary>
/// Result codes that can be returned from Cozmo functions.
/// </summary>
public enum CozmoResult {
	OK = 0,
	
	// BindingResult
	BINDING_OK = 0,
	BINDING_ERROR_NOT_INITIALIZED = 0x8001,
	BINDING_ERROR_ALREADY_INITIALIZED = 0x8002,
	BINDING_ERROR_INVALID_CONFIGURATION = 0x8003,
	BINDING_ERROR_FAILED_INITIALIZATION = 0x8005,
	
	// Anki::Result
	//RESULT_FAIL                      = 0x00000001,
	//RESULT_FAIL_MEMORY               = 0x01000000,
	//RESULT_FAIL_OUT_OF_MEMORY        = 0x01000001,
	//RESULT_FAIL_UNINITIALIZED_MEMORY = 0x01000002,
	//RESULT_FAIL_ALIASED_MEMORY       = 0x01000003,
	//RESULT_FAIL_IO                   = 0x02000000,
	//RESULT_FAIL_IO_TIMEOUT           = 0x02000001,
	//RESULT_FAIL_IO_CONNECTION_CLOSED = 0x02000002,
	//RESULT_FAIL_INVALID_PARAMETER    = 0x03000000,
	//RESULT_FAIL_INVALID_OBJECT       = 0x04000000,
	//RESULT_FAIL_INVALID_SIZE         = 0x05000000,

	CSHARP_NOT_AVAILABLE = 0x00040001,
}

public static class CozmoBinding {

	[DllImport ("__Internal")]
	public static extern bool cozmo_has_log(out int receive_length);

	[DllImport ("__Internal")]
	public static extern void cozmo_pop_log(StringBuilder buffer, int max_length);

	[DllImport("__Internal")]
	public static extern int cozmo_game_host_create (string configurationData);

	[DllImport("__Internal")]
	public static extern int cozmo_game_host_destroy();
	
	[DllImport("__Internal")]
	public static extern int cozmo_game_update(float currentTime);
}

