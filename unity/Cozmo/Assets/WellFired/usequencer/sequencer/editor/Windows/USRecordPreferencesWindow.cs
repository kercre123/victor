using UnityEngine;
using UnityEditor;
using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	public class USRecordPreferencesWindow : EditorWindow 
	{
		[MenuItem("Edit/uSequencer/Recording Preferences")]
		public static void OpenWindow()
		{
			USRecordPreferencesWindow window = EditorWindow.GetWindow(typeof(USRecordPreferencesWindow), true, "Recording Preferences", true) as USRecordPreferencesWindow;	    
			window.position = new Rect(150, 150, 500, 220);
		}
		
		private void OnEnable()
		{
			USRecordRuntimePreferences.Reset();
		}
		
		private void OnDestroy()
		{
			USRecordRuntimePreferences.Destroy();
		}
		
		private void OnGUI()
		{
			EditorGUILayout.HelpBox("To set the resolution, this has to be done within Unity. " +
				"\n\nFirst select Edit / Project Settings / Player. Then, ensure you set the Default Screen Width and Height in Resolution and Presentation for the PC and Mac Standalone in the inspector. " +
				"\n\nThen change the aspect ratio using the drop down box at top left of the game Window. You'll then need to resize the window so that there is a border around the rendering area.", MessageType.Info);
			
			List<PresetInfo> presetsList = USRecordRuntimePreferences.GetPresetInfo();
			string[] presetNames = presetsList.Select(preset => preset.Name).ToArray();
			int newSelectedPreset = EditorGUILayout.Popup("Preset", USRecordRuntimePreferences.SelectedPreset, presetNames);
			if(newSelectedPreset != USRecordRuntimePreferences.SelectedPreset)
			{
				USRecordRuntimePreferences.SetNewPreset(newSelectedPreset);
			}
			
			USRecordRuntimePreferences.PresetName = EditorGUILayout.TextField("Preset Name", USRecordRuntimePreferences.PresetName);
			using(new WellFired.Shared.GUIBeginHorizontal())
			{
				USRecordRuntimePreferences.CapturePath = EditorGUILayout.TextField("Capture Path", USRecordRuntimePreferences.CapturePath);
				if(GUILayout.Button("Select", GUILayout.Width(60)))
					USRecordRuntimePreferences.CapturePath = EditorUtility.OpenFolderPanel("Select Image Path", "", "");
			}
			
			USRecordRuntimePreferences.UpscaleAmount 	= (USRecord.Upscaling)			EditorGUILayout.EnumPopup("Upscale Amount", USRecordRuntimePreferences.UpscaleAmount);
			USRecordRuntimePreferences.FrameRate 		= (USRecord.FrameRate)			EditorGUILayout.EnumPopup("Capture FrameRate", USRecordRuntimePreferences.FrameRate);
			
			using(new WellFired.Shared.GUIBeginHorizontal())
			{
				GUILayout.FlexibleSpace();
				if(GUILayout.Button("Save As New Preset"))
				{
					USRecordRuntimePreferences.SaveAsNewPreset(USRecordRuntimePreferences.PresetName, USRecordRuntimePreferences.CapturePath, USRecordRuntimePreferences.UpscaleAmount, USRecordRuntimePreferences.FrameRate);
					USRecordRuntimePreferences.SelectedPreset = presetsList.Count();
				}
				if(GUILayout.Button("Apply to existing Preset"))
				{
					presetsList[USRecordRuntimePreferences.SelectedPreset].Name = USRecordRuntimePreferences.PresetName;
					presetsList[USRecordRuntimePreferences.SelectedPreset].CapturePath = USRecordRuntimePreferences.CapturePath;
					presetsList[USRecordRuntimePreferences.SelectedPreset].UpscaleAmount = USRecordRuntimePreferences.UpscaleAmount;
					presetsList[USRecordRuntimePreferences.SelectedPreset].FrameRate = USRecordRuntimePreferences.FrameRate;
					
					USRecordRuntimePreferences.SavePresets(presetsList);
				}
				if(GUILayout.Button("Delete Preset"))
				{
					USRecordRuntimePreferences.DeletePreset();
				}
				GUILayout.FlexibleSpace();
			}
		}
	}
}