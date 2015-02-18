//----------------------------------------------
//            ScreenRecorder
// Copyright © 2011-2012 Andriy Grygorenko
//----------------------------------------------
using UnityEngine;
using System;
using System.Collections;
using System.Runtime.InteropServices;

/** ScreenRecorder is primary class that you can use create time-based audiovisual media.
  *ScreenRecorder provides an c# interface you use to work with audiovisual data recording. 
  *In other words ScreenRecorder will write to audiovisual container (mp4 file) content that scene renders into RenderingTarget allocated by ScreenRecorder.
  *You can choose write only visual content (auto switched off) or audiovisual using ScreenRecorder.recordAudio public member.
  *You can choose video resolution and framerate using ScreenRecorder.videoResolution and ScreenRecorder.frameRate public member.
  *Audiovisual data recording recording it self is done on a level of plugin represented by following interface: 
  *InitScreenRecorder, StartScreenRecorder, IsStartedScreenRecorder, ScreenRecorderPushFrame, StopScreenRecorder, ReleaseScreenRecorder.
  *Be aware that the plugin utilize both CPU and GPU, so it changes OpenGL state as result of functions calls:
  *InitScreenRecorder, StartScreenRecorder, ScreenRecorderPushFrame, StopScreenRecorder.
  *To guarantee thread safety and OpenGL state consistency, 
  *these functions executed in a coroutine after end of the frame (look at WaitForEndOfFrame()) in OpenGL thread (look at GL.IssuePluginEvent).
  *Supported platforms: iOS 5.0 and later, MacOSX 10.7 for development purpose only.
 */
public class ScreenRecorder : MonoBehaviour
{
	
	#region NESTED
	/** Video container type.
	 *Currently supported only mp4.
	 */
	public enum VideoWriterCodecType
	{
		MP4 = 0
		//not supported yet
		//Jpeg
	}
	
	/** Video resolution. 
	 *Defines supported video resolutions. 
	 *Please keep in mind that bigger resolution has bigger impact on ScreenRecorder perforce.
	 *Exact resolution calculated at runtime, depends on device screen size.
	 */
	public enum VideoResolution
	{	/**Result video width is equal to Screen.width and height is equal to Screen.height.*/
		Best = 0,
		/**The same as VideoResolution.Best with restriction to be less than 720p.*/
		High,
		/**Result video resolution will be smaller or equal to 672x448 (448x672 in a portrait mode).*/
		Medium,
		/**Result video resolution will be smaller or equal to 360x240 (240x360 in a portrait mode).*/
		Low, 
		/**Same as Medium.*/
		Default 
	}
	
	private const int eventsStartPoint = 0xfef;
	/** Plugin events. 
	 *Defines plugin event ids. Used with GL.IssuePluginEvent to perform thread safe OpenGL calls.
	 */
	private enum AsyncEvents
	{
		InitScreenRecorderEvent = eventsStartPoint,
		StartScreenRecorderEvent,
		ScreenRecorderPushFrameEvent,
		StopScreenRecorderEvent,
		ReleaseScreenRecorderEvent
	};
	#endregion

	#region PUBLIC_MEMBERS
	/** Video resolution. 
	 *Use this public member to define required video resolution.
	 */
	public VideoResolution videoResolution = VideoResolution.Default;
	/** Video frame rate.
	 *Use this public member to define required video frame rate.
	 *Can be any number between 0 to 60.
	 */
	public int frameRate = 30;
	/** Video container type.
	 *Use this public member to define required video container type.
	 *Currently supported only mp4.
	 */
	public VideoWriterCodecType codecType = VideoWriterCodecType.MP4;
	/** Set whether need to add audio stream from device microphone into video.
	 *Use this public member to add audio stream into video.
	 */
	public bool recordAudio = true;
	/** Set whether need to save video into camera roll.
	 *Use this public member to setup video recorder to save video into device camera roll after it has been finalized.
	 */
	public bool saveToCameraRoll = true;
	/** Set whether need to stop video recording at device pause event.
	 *If saveWhenPause is set than ScreenRecorder will stop reordering and finalize video at OnApplicationPause event.
	 */
	public bool saveWhenPause = true;
	/** Make video orientation to look right for Augmented Reality applications.
	 *Video orientated according to game orientation looks wrong with augmented reality games. To fix it set augmentedRealityMode to true.
	 */
	public bool augmentedRealityMode = false;
	/** Result video file name.
	 *Result video file name. To get full path to file use ScreenRecorder.videoFilePath (this will include file name).
	 */
	public string videoFileName = "";
	/** Scene initiales. 
	 *Use this public member to define custom scene initializers. Will cause switching off default scene initializer.
	 */
	public GameObject[] delegates;
	#endregion
	
	#region PRIVATE_MEMBERS
	private float quality = 1.3f;
	private float nextCaptureEvent;
	private float emitTimeInterval;
	private bool ifinitialized = false;
	private Camera[] targetCameras;

	/** Start recording event. 
	 *This event will be sent to all defined ScreenRecorder.deligates[] before recording is started. 
	 */
	private string startCallback = "OnStartRecording";
	
	/** Start recording event. 
	 *This event will be sent to all defined ScreenRecorder.deligates[] before recording is finalized. 
	 */
	private string stopCallback = "OnStopRecording";
	#endregion
	
	#region PUBLIC_METHODS
	
	public bool saveMovieToCameraRoll()
	{
		if(started)
			return false;
		
		string filePath = videoFilePath;
		IntPtr strPtr = IntPtr.Zero;
		int res = -1;
		try {
			strPtr = Marshal.StringToHGlobalAnsi(filePath);
			res = ScreenRecorderAPI.SaveMovieToCameraRoll (strPtr);
			} catch (Exception ex) {
				Console.WriteLine (ex);
			} finally {
				if (strPtr != IntPtr.Zero)
				Marshal.FreeHGlobal(strPtr);
			}
		return res == 0;
	}
	#endregion

	#region PUBLIC_PROPS
	
	/** Instance of ScreenRecorder. 
	 *Use this property to find instance of ScreenRecorder on a scene. Will return null if instance is not exist.
	 *@return instance of ScreenRecorder or null if can't find instance on a scene by name "ScreenRecorder".
	 */
	public static ScreenRecorder instance {
		get {
			GameObject obj = GameObject.Find ("ScreenRecorder");
			if (obj != null)
				return obj.GetComponent<ScreenRecorder> ();
			else
				return null;
		}
	}
	
	/** Rendering target. 
	 *Use this property to get ScreenRecorder rendering target which contains RenderTexture that have to be used as ScreenRecorder content provider.
	 *@return ScreenRecorder RenderingTarget.
	 */
	public RenderingTarget renderingTarget {
		get;
		protected set;
	}

	/** Video file path. 
	 *Use this property to get access to recorded video file. Video file can be accessed only after it is finalized.
	 *@return video file path including video file name.
	 */
	public string videoFilePath {
		get {
			IntPtr buf = IntPtr.Zero;
			string filePath = "";
			try {
				const int bufferSize = 1024;
				buf = Marshal.AllocHGlobal (bufferSize);
				ScreenRecorderAPI.ScreenRecorderBaseFolder (buf);
				filePath = Marshal.PtrToStringAnsi (buf);
				filePath = filePath + "/" + videoFileName;
			} catch (Exception ex) {
				Console.WriteLine (ex);
			} finally {
				if (buf != IntPtr.Zero)
					Marshal.FreeHGlobal (buf);
			}
			
			return filePath;			
		}
	}
	
	/** Checks whether ScreenRecorder is initialized.
	 *Use this property to check whether ScreenRecorder is initialized. If it is not any operations with ScreenRecorder will failed.
	 *@return true if ScreenRecorder is initialized false otherwise.
	 */
	public bool initialized {
		get {
			return ifinitialized;
		}
	}
	
	/** Checks whether ScreenRecorder is recording video.
	 *Use this property to check if ScreenRecorder is recording video. 
	 *@return true if ScreenRecorder is recording false otherwise.
	 */
	public bool started {
		get {
			return ScreenRecorderAPI.IsStartedScreenRecorder () == 1;
		}
	}
	
	/** Checks free space available for video reordering.
	 *@return free space available for video reordering in MB.
	 */
	public int freeSpace {
		get {
			return ScreenRecorderAPI.AvailableDiscSpace ();
		}
	}
	#endregion

	#region MONOBEHAVIOR
	void Awake ()
	{
		renderingTarget = ScriptableObject.CreateInstance<RenderingTarget> ();
		renderingTarget.Initialize ();
		StartCoroutine (initRecorder ());
	}

	void OnEnable ()
	{
		if (ifinitialized == false)
		{
			Debug.LogError("Error ScreenRecorder is not initialized.");
			enabled = false;
			return;
		}
		StartCoroutine (startRecorder ());
		setupRecordingScene ();
	}
	
	void OnDisable ()
	{
		if (ifinitialized == false)
		{
			Debug.LogError("Error ScreenRecorder is not initialized.");
			return;
		}
		StartCoroutine (stopRecorder ());
		teardownRecordingScene ();
	}
	
	void OnDestroy ()
	{
		// just make sure object is destroyed in a right thread in Editor
		Destroy (renderingTarget);
		releaseRecorder ();
	}
	
	void OnApplicationPause (bool pause)
	{
#if (UNITY_IPHONE && !UNITY_EDITOR)		
		if(saveWhenPause)
		{
			if(pause == true){
				if(enabled)
					enabled = false;	
			}
		}
#endif
	}
	
	#endregion
	
	#region PRIVATE_METHODS
	private void setupRecordingScene ()
	{
		if (notifyDeligates (startCallback))
			return;
		
		targetCameras = Camera.allCameras;
		
		for (int i=0; i< targetCameras.Length; ++i) { 
			Camera c = targetCameras [i];
			if (c != camera 
				&& c != null 
				&& c.targetTexture == null) {
				c.targetTexture = renderingTarget.target;
			}
		}
	}

	private void teardownRecordingScene ()
	{
		if (notifyDeligates (stopCallback))
			return;
		
		for (int i=0; i< targetCameras.Length; ++i) { 
			Camera c = targetCameras [i];
			if (c != camera && c != null)
				c.targetTexture = null;
		}
	}
	
	private bool notifyDeligates (string handler)
	{
		if (delegates == null || delegates.Length == 0)
			return false;
		
		foreach (GameObject d in delegates) {
			d.SendMessage (handler, renderingTarget.target);
		}
		return true;
	}
	
	static private int alignBy8 (int v)
	{
		return v += (8 - v % 8) % 8;
	}
	
	static private void mapToScreenResolution (ref int width, ref int height, int screenWidth, int screenHeight)
	{
		if (screenWidth > screenHeight) {
			width = alignBy8 (Mathf.Min (width, screenWidth));
			height = alignBy8 ((int)((float)screenHeight * (float)width / (float)screenWidth));
		} else {
			height = alignBy8 (Mathf.Min (width, screenHeight));
			width = alignBy8 ((int)((float)screenWidth * (float)height / (float)screenHeight));
		}
		
		
	}
	
	public static Resolution videoResolutionForQualityAndScreenSize (VideoResolution vr, int  screenWidth, int screenHeight)
	{
		Resolution resolution = new Resolution ();
		switch (vr) {
		case VideoResolution.Best:
			resolution.width = alignBy8 (screenWidth);
			resolution.height = alignBy8 (screenHeight);
			break;

		case VideoResolution.High:
			{
				int width = 1280;
				int height = 720;
				mapToScreenResolution (ref width, ref height, screenWidth, screenHeight);
				resolution.width = width;
				resolution.height = height;
			}
			break;
				
		case VideoResolution.Medium:
		case VideoResolution.Default:
			{
				int width = 672;
				int height = 448;
				mapToScreenResolution (ref width, ref height, screenWidth, screenHeight);
				resolution.width = width;
				resolution.height = height;
			}
			break;
		case VideoResolution.Low:
			{
				int width = 360;
				int height = 240;
				mapToScreenResolution (ref width, ref height, screenWidth, screenHeight);
				resolution.width = width;
				resolution.height = height;
			}
			break;
		}
			
		return resolution;
	}
	
	public Resolution videoResolutionDetails {
		get {
			return videoResolutionForQualityAndScreenSize (videoResolution, Screen.width, Screen.height);
		}
	}
	#endregion
	
	#region COROUINES
	private IEnumerator initRecorder ()
	{
		yield return new WaitForEndOfFrame();	
		
		Resolution resolution = videoResolutionDetails;		
		int res = ScreenRecorderAPI.InitScreenRecorder (resolution.width, resolution.height, frameRate, quality, (int)codecType);
		
		if (res != 0)
		{
			ifinitialized = false;
			Debug.LogError("Error ScreenRecorder initialization.");
			yield break;	
		}
#if (UNITY_IPHONE && !UNITY_EDITOR)		
		GL.InvalidateState ();
#endif		
		GL.IssuePluginEvent ((int)AsyncEvents.InitScreenRecorderEvent);
		ifinitialized = true;
		yield break;
	}
	
	private IEnumerator startRecorder ()
	{
		if(ifinitialized == false)
		{
			Debug.LogError("Error ScreenRecorder is not initialized.");
			yield break;	
		}

		yield return new WaitForEndOfFrame();

		IntPtr lpData = Marshal.StringToHGlobalAnsi (videoFileName);
		int res = ScreenRecorderAPI.StartScreenRecorder (lpData, recordAudio ? 1 : 0, augmentedRealityMode ? 1 : 0);	
		Marshal.FreeHGlobal (lpData);
		if (res != 0)
		{
			Debug.LogError("Error ScreenRecorder start.");
			yield break;	
		}

#if (UNITY_IPHONE && !UNITY_EDITOR)		
		GL.InvalidateState ();
#endif		
		GL.IssuePluginEvent ((int)AsyncEvents.StartScreenRecorderEvent);
		nextCaptureEvent = 0;
		emitTimeInterval = 1.0f / (float)frameRate;
		
		// start pushing  frames
		StartCoroutine ("pushFrames");
		yield break;
	}
	
	private IEnumerator stopRecorder ()
	{
		if(ifinitialized == false)
		{
			Debug.LogError("Error ScreenRecorder is not initialized.");
			yield break;	
		}
		
		//stop pushing frames
		StopCoroutine ("pushFrames");
		
		yield return new WaitForEndOfFrame();	
		int res = ScreenRecorderAPI.StopScreenRecorder (saveToCameraRoll);
		if (res != 0)
		{
			Debug.LogError("Error ScreenRecorder stop.");
			yield break;	
		}
		
#if (UNITY_IPHONE && !UNITY_EDITOR)		
		GL.InvalidateState ();
#endif		
		GL.IssuePluginEvent ((int)AsyncEvents.StopScreenRecorderEvent);
		
		yield break;
	}

	private void releaseRecorder ()
	{
		if(ifinitialized == false)
		{
			Debug.LogError("Error ScreenRecorder is not initialized.");
			return;	
		}
		
		//yield return new WaitForEndOfFrame();	
		int res = ScreenRecorderAPI.ReleaseScreenRecorder ();	
		if (res != 0)
		{
			Debug.LogError("Error ScreenRecorder release.");
			return;	
		}

#if (UNITY_IPHONE && !UNITY_EDITOR)		
		GL.InvalidateState ();
#endif		
		GL.IssuePluginEvent ((int)AsyncEvents.ReleaseScreenRecorderEvent);
	}
	
	IEnumerator pushFrames ()
	{
		if(ifinitialized == false)
		{
			Debug.LogError("Error ScreenRecorder is not initialized.");
			yield break;	
		}
		
		while (true) {
			yield return new WaitForEndOfFrame();	
		
			//blit texture if not delegates registered
			if (delegates == null || delegates.Length == 0)
				renderingTarget.blit ();
			
			if ((nextCaptureEvent - Time.time - 0.01f) > 0f)
				continue;

			nextCaptureEvent = Time.time + emitTimeInterval;	
			int glid = renderingTarget.targetID;
			ScreenRecorderAPI.ScreenRecorderPushFrame (glid);
#if (UNITY_IPHONE && !UNITY_EDITOR)		
			GL.InvalidateState ();
#endif		
			//this command works only for desctops
			GL.IssuePluginEvent ((int)AsyncEvents.ScreenRecorderPushFrameEvent);
		}
	}
	#endregion
	
	struct ScreenRecorderAPI
	{
		#region NATIVE_FUNCTIONS
#if ((UNITY_EDITOR || UNITY_STANDALONE_OSX || UNITY_IPHONE) && !UNITY_OTHER_PLATFORM_DEBUG)
		[DllImport(ScreenRecorderSpace.ScreenRecorderCommon.DllImportDefinition)]
	    public static extern int InitScreenRecorder(int width, int height, int fps, float compression, int vrType);
	
		[DllImport(ScreenRecorderSpace.ScreenRecorderCommon.DllImportDefinition)]
	    public static extern int StartScreenRecorder(IntPtr videoFileName, int withAudio, int withARmode);
		
		[DllImport(ScreenRecorderSpace.ScreenRecorderCommon.DllImportDefinition)]
	    public static extern int StopScreenRecorder(bool saveToCameraRoll);
		
		[DllImport(ScreenRecorderSpace.ScreenRecorderCommon.DllImportDefinition)]
	    public static extern int ReleaseScreenRecorder();
		
		[DllImport(ScreenRecorderSpace.ScreenRecorderCommon.DllImportDefinition)]
		public static extern int ScreenRecorderPushFrame (int textureID);
		
		[DllImport(ScreenRecorderSpace.ScreenRecorderCommon.DllImportDefinition)]
		public static extern int ScreenRecorderBaseFolder(IntPtr folderName);
		
		[DllImport(ScreenRecorderSpace.ScreenRecorderCommon.DllImportDefinition)]
		public static extern int IsStartedScreenRecorder();
		
		[DllImport(ScreenRecorderSpace.ScreenRecorderCommon.DllImportDefinition)]
		public static extern int AvailableDiscSpace();
		
		[DllImport(ScreenRecorderSpace.ScreenRecorderCommon.DllImportDefinition)]
		public static extern int SaveMovieToCameraRoll(IntPtr filePath);
#else
		public static int InitScreenRecorder (int width, int height, int fps, float compression, int vrType)
		{
			return 0;
		}

		public static int StartScreenRecorder (IntPtr videoFileName, int withAudio, int withARmode)
		{
			return 0;
		}

		public static int StopScreenRecorder (bool saveToCameraRoll)
		{
			return 0;
		}

		public static int ReleaseScreenRecorder ()
		{
			return 0;
		}

		public static int ScreenRecorderPushFrame (int textureID)
		{
			return 0;
		}
		
		public static int ScreenRecorderBaseFolder (IntPtr folderName)
		{
			return 0;
		}
		
		public static int AvailableDiscSpace ()
		{
			return 0;
		}
		
		public static int IsStartedScreenRecorder ()
		{
			return 0;
		}
#endif
		#endregion // NATIVE_FUNCTIONS
	}	
}
