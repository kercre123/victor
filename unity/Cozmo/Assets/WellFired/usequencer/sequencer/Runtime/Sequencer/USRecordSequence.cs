using UnityEngine;
using System.Collections;

namespace WellFired
{
	public class USRecordSequence : MonoBehaviour 
	{
		#region Member Variables
		private bool isRecording = false;
		private bool recordOnStart = false;
		
		private int imageCount = 0;
		
		private int captureFrameRate = 60;
		
		private int upscaleAmount = 1;
		
		private string capturePath = "";
		#endregion
		
		#region Attributes
		public int CaptureFrameRate
		{
			set
			{
				captureFrameRate = Mathf.Clamp(value, 1, 60);
			}
		}
		
		public int UpscaleAmount
		{
			set
			{
				upscaleAmount = Mathf.Clamp(value, 1, 8);
			}
		}
		
		public string CapturePath
		{
			set
			{
				capturePath = value;
			}
		}
		
		public bool RecordOnStart
		{
			set
			{
				recordOnStart = value;
			}
		}
		#endregion
			
		private void Start() 
		{
			imageCount = 0;
			isRecording = recordOnStart;
		}
		
		private void LateUpdate() 
		{	
			Time.captureFramerate = captureFrameRate;
			
			if(isRecording)
			{
				Application.CaptureScreenshot(capturePath + "/Screenshot_" + imageCount + ".png", upscaleAmount);
				imageCount++;
			}
		}
		
		public void StartRecording()
		{
			isRecording = true;
		}
		
		public void PauseRecording()
		{
			isRecording = false;
		}
		
		public void StopRecording()
		{
			isRecording = false;
			imageCount = 0;
		}
	}
}