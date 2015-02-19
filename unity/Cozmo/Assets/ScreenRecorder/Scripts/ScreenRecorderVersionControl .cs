//----------------------------------------------
//            ScreenRecorder
// Copyright © 2011-2012 Andriy Grygorenko
//----------------------------------------------

using System;
using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;

namespace ScreenRecorderSpace
{
/** ScreenRecorder version control functionality. 
 * Use this class to determine ScreenRecorder plugin version.
 */	
	public class ScreenRecorderVersionControl
	{
		private static int mMajorVersion = -1;
		private static int mMinorVersion = -1;
		private static int mBuildNumber = -1;
		private static string mVersionString = "";
	
	#region PUBLIC_METHODS
	  /** Checks ScreenRecorder plugin major version number.
	   *@return major version number.
	   */
		public static int majorVersion {
			get {
				if (mMajorVersion == -1) {
					mMajorVersion = ScreenRecorderVersionAPI.ScreenRecorderMajorVersion ();
				}
				return mMajorVersion;
			}
		}
	
	  /** Checks ScreenRecorder plugin minor version number.
	   *@return minor version number.
	   */
		public static int minorVersion {
			get {
				if (mMinorVersion == -1) {
					mMinorVersion = ScreenRecorderVersionAPI.ScreenRecorderMinorVersion ();
				}
				return mMinorVersion;
			}
		}

	  /** Checks ScreenRecorder plugin build number.
	   *@return build number.
	   */
		public static int buildNumber {
			get {

				if (mMinorVersion == -1) {
					mBuildNumber = ScreenRecorderVersionAPI.ScreenRecorderBuildNumber ();
				}
				return mBuildNumber;
			}
		}
	
	  /** Checks ScreenRecorder plugin version string.
	   *@return version string.
	   */
		public static string version {
			get {
				if (mVersionString.Length != 0)
					return mVersionString;
			
				IntPtr buf = IntPtr.Zero;
				string versionString = "";
				try {
					const int bufferSize = 1024;
					buf = Marshal.AllocHGlobal (bufferSize);
					ScreenRecorderVersionAPI.ScreenRecorderVersion (buf, bufferSize);
					versionString = Marshal.PtrToStringAnsi (buf);
				} catch (Exception ex) {
					Console.WriteLine (ex);
				} finally {
					if (buf != IntPtr.Zero)
						Marshal.FreeHGlobal (buf);
				}
				mVersionString = versionString;
				return versionString;
			}
		}

	#endregion // MONOBEHAVIOR

		struct ScreenRecorderVersionAPI
		{
		#region NATIVE_FUNCTIONS
#if ((UNITY_EDITOR || UNITY_STANDALONE_OSX || UNITY_IPHONE) && !NESTOR_UNITY_OTHER_PLATFORM_DEBUG)
		[DllImport(ScreenRecorderSpace.ScreenRecorderCommon.DllImportDefinition)]
	    public static extern void ScreenRecorderVersion(IntPtr versionBuffer, int bufferSize);
	
		[DllImport(ScreenRecorderSpace.ScreenRecorderCommon.DllImportDefinition)]
	    public static extern int ScreenRecorderMajorVersion();
		
		[DllImport(ScreenRecorderSpace.ScreenRecorderCommon.DllImportDefinition)]
	    public static extern int ScreenRecorderMinorVersion();
		
		[DllImport(ScreenRecorderSpace.ScreenRecorderCommon.DllImportDefinition)]
	    public static extern int ScreenRecorderBuildNumber();
#else
			public static void ScreenRecorderVersion (IntPtr versionBuffer, int bufferSize)
			{
			}

			public static int ScreenRecorderMajorVersion ()
			{
				return 0;
			}

			public static int ScreenRecorderMinorVersion ()
			{
				return 0;
			}

			public static int ScreenRecorderBuildNumber ()
			{
				return 0;
			}
#endif
		#endregion // NATIVE_FUNCTIONS
		}
	}

}