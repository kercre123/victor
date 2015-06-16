using UnityEngine;
using System.Collections;

namespace WellFired
{
	public class USRecord
	{
		public enum PlayerResolution
		{
			_1920x1080,
			_1280x720,
			_960x540,
			_854x480,
			_720x576,
			_640x480,
			_Custom,
		};
		
		public enum FrameRate
		{
			_24,
			_25,
			_30,
			_50,
			_60,
		};
		
		public enum Upscaling
		{
			_1,
			_2,
			_4,
			_8,
		};
		
		public static int GetResolutionX()
		{		
			switch(USRecordRuntimePreferences.Resolution)
			{
			case PlayerResolution._1920x1080:
				return 1920;
			case PlayerResolution._1280x720:
				return 1280;
			case PlayerResolution._960x540:
				return 960;
			case PlayerResolution._854x480:
				return 854;
			case PlayerResolution._720x576:
				return 720;
			case PlayerResolution._640x480:
				return 640;
			case PlayerResolution._Custom:
				return 60;
			}
			
			return 960;
		}
			
		public static int GetResolutionY()
		{		
			switch(USRecordRuntimePreferences.Resolution)
			{
			case PlayerResolution._1920x1080:
				return 1080;
			case PlayerResolution._1280x720:
				return 720;
			case PlayerResolution._960x540:
				return 540;
			case PlayerResolution._854x480:
				return 480;
			case PlayerResolution._720x576:
				return 576;
			case PlayerResolution._640x480:
				return 480;
			case PlayerResolution._Custom:
				return 60;
			}
			
			return 420;
		}
		
		public static int GetFramerate()
		{
			switch(USRecordRuntimePreferences.FrameRate)
			{
			case FrameRate._24:
				return 24;
			case FrameRate._25:
				return 25;
			case FrameRate._30:
				return 30;
			case FrameRate._50:
				return 50;
			case FrameRate._60:
				return 60;
			}
			
			return 60;
		}
		
		public static int GetUpscaleAmount()
		{		
			switch(USRecordRuntimePreferences.UpscaleAmount)
			{
			case Upscaling._1:
				return 1;
			case Upscaling._2:
				return 2;
			case Upscaling._4:
				return 4;
			case Upscaling._8:
				return 8;
			}
			
			return 2;
		}
	}
}