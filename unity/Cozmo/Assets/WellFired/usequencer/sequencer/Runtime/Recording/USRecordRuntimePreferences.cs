using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	public static class USRecordRuntimePreferences
	{
		#region Member Variables
		private static string resolutionPref 						= "uSequencer-RecordPrefs-CaptureResolution";
		private static string frameRatePref		 					= "uSequencer-RecordPrefs-CaptureFrameRate";
		private static string upscaleAmountPref 					= "uSequencer-RecordPrefs-UpscaleAmount";
		private static string pathPref		 						= "uSequencer-RecordPrefs-CapturePath";
		private static string presetNamePref						= "uSequencer-RecordPrefs-PresetName";
		private static string presetPref							= "uSequencer-RecordPrefs-PresetPrefData";
		private static string selectedPresetPref					= "uSequencer-RecordPrefs-SelectedPreset";
		
		private static USRecord.PlayerResolution defaultResolution 	= USRecord.PlayerResolution._960x540;
		private static USRecord.FrameRate defaultFrameRate 			= USRecord.FrameRate._60;
		private static USRecord.Upscaling defaultUpscaling 			= USRecord.Upscaling._2;
		private static string defaultPreset							= "(Default;c:/Blah/Save_Loc;_2;_60)";
		private static int defaultSelectedPreset					= 0;
		#endregion
		
		#region Properties
		public static int SelectedPreset
		{
			get
			{
				if(!PlayerPrefs.HasKey(selectedPresetPref))
					PlayerPrefs.SetInt(selectedPresetPref, defaultSelectedPreset);
				
				return PlayerPrefs.GetInt(selectedPresetPref);
			}
			set
			{
				PlayerPrefs.SetInt(selectedPresetPref, value);
			}
		}
		
		public static string PresetName
		{
			get
			{
				if(!PlayerPrefs.HasKey(presetNamePref))
					PlayerPrefs.SetString(presetNamePref, "default");
				
				return PlayerPrefs.GetString(presetNamePref);
			}
			set
			{
				PlayerPrefs.SetString(presetNamePref, value);
			}
		}
		
		public static USRecord.PlayerResolution Resolution
		{
			get
			{
				if(!PlayerPrefs.HasKey(resolutionPref))
					PlayerPrefs.SetInt(resolutionPref, (int)defaultResolution);
				
				return (USRecord.PlayerResolution)PlayerPrefs.GetInt(resolutionPref);
			}
			set
			{
				PlayerPrefs.SetInt(resolutionPref, (int)value);
			}
		}
		
		public static USRecord.FrameRate FrameRate
		{
			get
			{
				if(!PlayerPrefs.HasKey(frameRatePref))
					PlayerPrefs.SetInt(frameRatePref, (int)defaultFrameRate);
				
				return (USRecord.FrameRate)PlayerPrefs.GetInt(frameRatePref);
			}
			set
			{
				PlayerPrefs.SetInt(frameRatePref, (int)value);
			}
		}
		
		public static USRecord.Upscaling UpscaleAmount
		{
			get
			{
				if(!PlayerPrefs.HasKey(upscaleAmountPref))
					PlayerPrefs.SetInt(upscaleAmountPref, (int)defaultUpscaling);
				
				return (USRecord.Upscaling)PlayerPrefs.GetInt(upscaleAmountPref);
			}
			set
			{
				PlayerPrefs.SetInt(upscaleAmountPref, (int)value);
			}
		}
		
		public static string CapturePath
		{
			get
			{
				if(!PlayerPrefs.HasKey(pathPref))
					PlayerPrefs.SetString(pathPref, GetDefaultCapturePath());
				
				return PlayerPrefs.GetString(pathPref);
			}
			set
			{
				PlayerPrefs.SetString(pathPref, value);
			}
		}
		
		private static string Presets
		{
			get
			{
				if(!PlayerPrefs.HasKey(presetPref))
					PlayerPrefs.SetString(presetPref, defaultPreset);
				
				return PlayerPrefs.GetString(presetPref);
			}
			set
			{
				PlayerPrefs.SetString(presetPref, value);
			}
		}
		#endregion
		
		public static string GetDefaultCapturePath()
		{
			int last = Application.dataPath.LastIndexOf("/");
			string defaultCapturePath = Application.dataPath.Remove(last);
			
			defaultCapturePath += "/Output";
			
			return defaultCapturePath;
		}

		public static void SetNewPreset(int newSelectedPreset)
		{
			List<PresetInfo> presetsList = USRecordRuntimePreferences.GetPresetInfo();
			SelectedPreset = newSelectedPreset;
			PresetName = presetsList[SelectedPreset].Name;
			CapturePath = presetsList[SelectedPreset].CapturePath;
			UpscaleAmount = presetsList[SelectedPreset].UpscaleAmount;
			FrameRate = presetsList[SelectedPreset].FrameRate;
		}

		public static void Reset()
		{
			List<PresetInfo> presetsList = GetPresetInfo();
			PresetName = presetsList[SelectedPreset].Name;
			CapturePath = presetsList[SelectedPreset].CapturePath;
			UpscaleAmount = presetsList[SelectedPreset].UpscaleAmount;
			FrameRate = presetsList[SelectedPreset].FrameRate;
		}

		public static void Destroy()
		{
			// Path is too short.
			if(CapturePath.Length == 0)
			{
				Debug.LogError(string.Format("Directory Path, specified in the uSequencer Preference Window is invalid, resetting to the default : {0}", GetDefaultCapturePath()));			
				CapturePath = GetDefaultCapturePath();
			}
		}

		public static List<PresetInfo> GetPresetInfo()
		{
			List<PresetInfo> presetInfo = new List<PresetInfo>();
			
			// Presets are defined as such
			// (Default;c:/Blah/Save_Loc;_2;_60)(another preset;Users/Info;e4;e25)(The Next Preset;Users/Another;e2;e25)
			string presetsData = Presets;
			if(presetsData.Length > 0)
			{
				string[] presetsSplitData = presetsData.Split(')');
				foreach(string presetData in presetsSplitData)
				{
					if(presetData.Length <= 0)
						continue;
					
					// presetData is defined as such
					// (Default;c:/Blah/Save_Loc;_2;_60)
					string data = presetData.Replace("(", "").Replace(")", "");
					string[] presetParams = data.Split(';');
					string presetName = presetParams[0];
					string savePath = presetParams[1];
					USRecord.Upscaling presetUpscaleValue = (USRecord.Upscaling)Enum.Parse(typeof(USRecord.Upscaling), presetParams[2]);
					USRecord.FrameRate presetFramerate = (USRecord.FrameRate)Enum.Parse(typeof(USRecord.FrameRate), presetParams[3]);
					
					presetInfo.Add(new PresetInfo(presetName, savePath, presetUpscaleValue, presetFramerate));
				}
			}
			
			return presetInfo;
		}
		
		public static void SaveAsNewPreset(string name, string capturePath, USRecord.Upscaling upscaleAmount, USRecord.FrameRate frameRate)
		{
			name = UniqifyName(name);
			// presetData is defined as such
			// (Default;c:/Blah/Save_Loc;_2;_60)
			string preset = String.Format("({0};{1};{2};{3})", name, capturePath, upscaleAmount, frameRate);
			Presets += preset;
		}
		
		public static void DeletePreset()
		{
			List<PresetInfo> presetsList = GetPresetInfo();
			if(presetsList.Count() <= 1)
				return;
			
			presetsList.Remove(presetsList[SelectedPreset]);
			Presets = "";
			foreach(PresetInfo presetInfo in presetsList)
				SaveAsNewPreset(presetInfo.Name, presetInfo.CapturePath, presetInfo.UpscaleAmount, presetInfo.FrameRate);
			
			if(SelectedPreset >= presetsList.Count())
				SelectedPreset--;
			
			PresetName = presetsList[SelectedPreset].Name;
			CapturePath = presetsList[SelectedPreset].CapturePath;
			UpscaleAmount = presetsList[SelectedPreset].UpscaleAmount;
			FrameRate = presetsList[SelectedPreset].FrameRate;
		}
		
		public static void SavePresets(List<PresetInfo> presets)
		{	
			Presets = "";
			foreach(PresetInfo presetInfo in presets)
			{
				SaveAsNewPreset(presetInfo.Name, presetInfo.CapturePath, presetInfo.UpscaleAmount, presetInfo.FrameRate);	
			}
		}
		
		private static string UniqifyName(string name)
		{
			bool unique = true;
			List<PresetInfo> presetsList = GetPresetInfo();
			foreach(PresetInfo info in presetsList)
			{
				if(info.Name == name)
					unique = false;
			}
			
			// it's not unique, try again.
			if(!unique)
			{
				name += "1";
				return UniqifyName(name);
			}
			
			return name;
		}
	}
}