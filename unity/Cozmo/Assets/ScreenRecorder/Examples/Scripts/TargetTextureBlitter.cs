using UnityEngine;
using System.Collections;

public class TargetTextureBlitter : MonoBehaviour 
{
	//just find screen recorder and blit content on screen
	void OnPostRender () 
	{
		ScreenRecorder recorder = ScreenRecorder.instance;
		if(    recorder == null 
			|| recorder.initialized == false
			|| recorder.enabled == false)
			return;
		
		recorder.renderingTarget.blit();
	}	
}
