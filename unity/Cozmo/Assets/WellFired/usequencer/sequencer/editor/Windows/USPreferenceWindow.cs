using UnityEngine;
using UnityEditor;
using System;
using System.Collections;

namespace WellFired
{
	public class USPreferenceWindow : EditorWindow 
	{
		[MenuItem("Edit/uSequencer/General Preferences")]
		public static void OpenWindow()
		{
			USPreferenceWindow window = EditorWindow.GetWindow(typeof(USPreferenceWindow), true, "Preferences", true) as USPreferenceWindow;	    
			window.position = new Rect(150, 150, 500, 180);
		}

		private static string defaultEventEditorWidthPref 	= "uSequencer-Prefs-DefaultEventEditorWidth";
		private static string zoomFactor 					= "uSequencer-Prefs-ZoomFactor-test";
		private static string zoomInvert					= "uSequencer-Prefs-ZoomInvert";
		private static string scrollFactor 					= "uSequencer-Prefs-ScrollFactor-test";
		private static string dataDirectory					= "uSequencer-Prefs-DefaultPrefabDirectory";
		private static string hasReadDocumentationPrefabs	= "uSequencer-Prefs-Documentation-Read-Warning-For-Prefabs";
		private static string shouldRenderInPlayMode		= "uSequencer-Prefs-Should-Render-In-Play-Mode";
		private static string autoKeyframing				= "uSequencer-Prefs-Auto-Keyframing";
		private static string renderHierarchyGizmos			= "uSequencer-Prefs-Render-Hierarchy-Gizmos";
		
		private static float defaultEventEditorWidth 		= 100.0f;
		private static float defaultOSXZoomFactor 			= 0.001f;
		private static float defaultWinZoomFactor 			= 0.01f;
		private static bool defaultZoomInvert				= false;
		private static float defaultScrollFactor 			= 1.0f;
		private static string defaultDataDirectory			= "uSequencerData";
		private static bool defaultShouldRenderInPlayMode	= true;
		private static bool defaultAutoKeyframing			= true;
		private static bool defaultRenderHierarchyGizmos	= true;
		
		public static float DefaultEventEditorWidth
		{
			get
			{
				if(!EditorPrefs.HasKey(defaultEventEditorWidthPref))
					EditorPrefs.SetFloat(defaultEventEditorWidthPref, defaultEventEditorWidth);
				
				return EditorPrefs.GetFloat(defaultEventEditorWidthPref);
			}
			private set
			{
				EditorPrefs.SetFloat(defaultEventEditorWidthPref, value);
			}
		}
		
		public static float ZoomFactor
		{
			get
			{
				if(!EditorPrefs.HasKey(zoomFactor))
				{
					if(Application.platform == RuntimePlatform.OSXEditor)
						EditorPrefs.SetFloat(zoomFactor, defaultOSXZoomFactor);
					else if (Application.platform == RuntimePlatform.WindowsEditor)
						EditorPrefs.SetFloat(zoomFactor, defaultWinZoomFactor);
				}
				
				return EditorPrefs.GetFloat(zoomFactor);
			}
			private set
			{
				EditorPrefs.SetFloat(zoomFactor, value);
			}
		}
		
		public static bool ZoomInvert
		{
			get
			{
				if(!EditorPrefs.HasKey(zoomInvert))
					EditorPrefs.SetBool(zoomInvert, defaultZoomInvert);
				
				return EditorPrefs.GetBool(zoomInvert);
			}
			private set
			{
				EditorPrefs.SetBool(zoomInvert, value);
			}
		}
		
		public static float ScrollFactor
		{
			get
			{
				if(!EditorPrefs.HasKey(scrollFactor))
					EditorPrefs.SetFloat(scrollFactor, defaultScrollFactor);
				
				return EditorPrefs.GetFloat(scrollFactor);
			}
			private set
			{
				EditorPrefs.SetFloat(scrollFactor, value);
			}
		}
		
		public static string DataDirectory
		{
			get
			{
				if(!EditorPrefs.HasKey(dataDirectory))
					EditorPrefs.SetString(dataDirectory, defaultDataDirectory);
				
				return EditorPrefs.GetString(dataDirectory);
			}
			private set
			{
				EditorPrefs.SetString(dataDirectory, value);
			}
		}
		
		public static bool HasReadDocumentationPrefabs
		{
			get
			{
				if(!EditorPrefs.HasKey(hasReadDocumentationPrefabs))
					EditorPrefs.SetBool(hasReadDocumentationPrefabs, false);
				
				return EditorPrefs.GetBool(hasReadDocumentationPrefabs);
			}
			set
			{
				EditorPrefs.SetBool(hasReadDocumentationPrefabs, value);
			}
		}
		
		public static bool ShouldRenderInPlayMode
		{
			get
			{
				if(!EditorPrefs.HasKey(shouldRenderInPlayMode))
					EditorPrefs.SetBool(shouldRenderInPlayMode, defaultShouldRenderInPlayMode);
				
				return EditorPrefs.GetBool(shouldRenderInPlayMode);
			}
			set
			{
				EditorPrefs.SetBool(shouldRenderInPlayMode, value);
			}
		}
		
		public static bool AutoKeyframing
		{
			get
			{
				if(!EditorPrefs.HasKey(autoKeyframing))
					EditorPrefs.SetBool(autoKeyframing, defaultAutoKeyframing);
				
				return EditorPrefs.GetBool(autoKeyframing);
			}
			set
			{
				EditorPrefs.SetBool(autoKeyframing, value);
			}
		}

		public static bool RenderHierarchyGizmos
		{
			get
			{
				if(!EditorPrefs.HasKey(renderHierarchyGizmos))
					EditorPrefs.SetBool(renderHierarchyGizmos, defaultRenderHierarchyGizmos);
				
				return EditorPrefs.GetBool(renderHierarchyGizmos);
			}
			set
			{
				EditorPrefs.SetBool(renderHierarchyGizmos, value);
			}
		}
		
		private void OnGUI()
		{
			DefaultEventEditorWidth = EditorGUILayout.FloatField("Default Event Editor Width", DefaultEventEditorWidth);
			
			if(Application.platform == RuntimePlatform.OSXEditor)
				ZoomFactor = EditorGUILayout.Slider("uSequencer Zoom Strength", ZoomFactor, 0.0f, 0.1f);
			else
				ZoomFactor = EditorGUILayout.Slider("uSequencer Zoom Strength", ZoomFactor, 0.0f, 1.0f);

			ZoomInvert = EditorGUILayout.Toggle("Invert Zoom", ZoomInvert);
			
			ScrollFactor = EditorGUILayout.Slider("uSequencer Scroll Strength", ScrollFactor, 0.0f, 100.0f);
			DataDirectory = EditorGUILayout.TextField("Default Data Path", DataDirectory);
			ShouldRenderInPlayMode = EditorGUILayout.Toggle("Should Render In Play Mode", ShouldRenderInPlayMode);
			AutoKeyframing = EditorGUILayout.Toggle("Enable Auto Keyframing", AutoKeyframing);
			RenderHierarchyGizmos = EditorGUILayout.Toggle("Render Hierarchy Gizmos", RenderHierarchyGizmos);

			if(GUILayout.Button("Reset Preferences"))
				ResetPreferences();
		}

		public void ResetPreferences()
		{
			if(Application.platform == RuntimePlatform.OSXEditor)
				ZoomFactor = defaultOSXZoomFactor;
			else if (Application.platform == RuntimePlatform.WindowsEditor)
				ZoomFactor = defaultWinZoomFactor;
			ZoomInvert = defaultZoomInvert;
			ScrollFactor = defaultScrollFactor;
			DataDirectory = defaultDataDirectory;
			ShouldRenderInPlayMode = defaultShouldRenderInPlayMode;
			AutoKeyframing = defaultAutoKeyframing;
			RenderHierarchyGizmos = defaultRenderHierarchyGizmos;
		}
	}
}