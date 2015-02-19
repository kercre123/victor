//----------------------------------------------
//            ScreenRecorder
// Copyright © 2011-2012 Andriy Grygorenko
//----------------------------------------------

using UnityEngine;
using System.Collections;

namespace ScreenRecorderSpace
{
/** ScreenRecorderCommon defined resources for ScreenRecorder internal purpose.
 */	
	public class ScreenRecorderCommon
	{
#if UNITY_EDITOR
/** Library name required by DllImport attribute for plugin code import.
 */	
  	public const string DllImportDefinition = "ScreenRecorder";
#elif UNITY_ANDROID
    public const string DllImportDefinition = "ScreenRecorder";
#elif UNITY_IPHONE
    public const string DllImportDefinition = "__Internal";
#else
	public const string DllImportDefinition = "ScreenRecorder";
#endif
	}
}