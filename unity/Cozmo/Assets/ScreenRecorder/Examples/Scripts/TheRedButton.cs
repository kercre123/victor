/*==============================================================================

	Copyright (c) Ogmento Reality Reinvented, Inc.

	All Rights Reserved.

==============================================================================*/

using UnityEngine;
using System.Collections;

public class TheRedButton : MonoBehaviour 
{
	#region PUBLIC_VARIABLES
	public Texture2D startButtonImage;
	public Texture2D stopButtonImage;
	public bool guiInVideo;
	#endregion
	
	
	#region PRIVATE_VARIABLES
	private Rect recordingButtonRect;
	private Vector2 tapPosition;
	#endregion
	
	#region MONOBEHAVIOR
	void Start()
	{
		float screenDiagonal = Mathf.Sqrt(Screen.width*Screen.width + Screen.height*Screen.height);
		int buttonSize = (int)(screenDiagonal/8.8f);
		recordingButtonRect = new Rect(Screen.width - buttonSize - 5, 5, buttonSize, buttonSize);
	}
	
	void OnScreenFingerTap( int fingerIndex, Vector2 fingerPos )
	{
		fingerPos = new Vector2(fingerPos.x, Screen.height - fingerPos.y);
		if(recordingButtonRect.Contains(fingerPos))
		{
			ScreenRecorder screenRecorder = ScreenRecorder.instance;
			if(screenRecorder == null)
			{
				Debug.LogError("Can not find screenrecorder instance");
			}
			
			if(screenRecorder.initialized == false)
				return;
			
			if(screenRecorder.enabled == false)
				startRecording(screenRecorder);
			else
				stopRecording(screenRecorder);
		}
	}


	private void startRecording(ScreenRecorder screenRecorder)
	{
		if(screenRecorder.enabled == false)
		{
			screenRecorder.enabled = true;
		}
	}

	private void stopRecording(ScreenRecorder screenRecorder)
	{
		if(screenRecorder.enabled == true)
		{
			screenRecorder.enabled = false;
		}
	}
	
	void Update()
	{
		if (Input.GetMouseButtonUp(0))
		{
			OnScreenFingerTap(0, new Vector2(Input.mousePosition.x, Input.mousePosition.y));
		}
	}

	void OnGUI()
	{
		ScreenRecorder screenRecorder = ScreenRecorder.instance;
		if(screenRecorder == null)
		{
			Debug.LogError("Can not find screenrecorder instance");
		}
		
		if(screenRecorder.started)
		{
			if(guiInVideo)
				RenderTexture.active = ScreenRecorder.instance.renderingTarget.target;
			
			GUI.DrawTexture(recordingButtonRect, stopButtonImage, ScaleMode.ScaleToFit, true);
			
			if(guiInVideo)
				RenderTexture.active = null;
		}
		else
			GUI.DrawTexture(recordingButtonRect, startButtonImage, ScaleMode.ScaleToFit, true);
	}
	
	#endregion
}
