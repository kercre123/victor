using UnityEngine;
using System.Collections;

public class ScreenRecorderDelegate : MonoBehaviour 
{
	void OnStartRecording (RenderTexture target)
	{
		if (camera == null)
		{
			Debug.LogError("ScreenRecorderDelegate have to be attached to camera");
			return;
		}
		
		camera.targetTexture = target;
	}
	
	void OnStopRecording (RenderTexture target)
	{
		if (camera == null)
		{
			Debug.LogError("ScreenRecorderDelegate have to be attached to camera");
			return;
		}

		camera.targetTexture = null;
	}
}
