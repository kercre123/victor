using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Reflection;

namespace WellFired
{
	public class UpgradeSequencesToNamespace : EditorWindow 
	{
		private class ScriptData
		{
			public Type ScriptType
			{
				get;
				private set;
			}
	
			public int OldFileID
			{
				get;
				private set;
			}
	
			public string OldGUID
			{
				get;
				private set;
			}
			
			public int NewFileID
			{
				get;
				private set;
			}
			
			public string NewGUID
			{
				get;
				private set;
			}
	
			public string GetFullOldDescription
			{
				get { return string.Format("m_Script: {0}fileID: {1}, guid: {2}", "{", OldFileID.ToString(), OldGUID); }
			}
			
			public string GetFullNewDescription
			{
				get { return string.Format("m_Script: {0}fileID: {1}, guid: {2}", "{", NewFileID.ToString(), NewGUID); }
			}
	
			public ScriptData(Type scriptType, int oldFileID, string oldGUID, int newFileID, string newGUID)
			{
				OldFileID = oldFileID;
				OldGUID = oldGUID;
	
				NewFileID = newFileID;
				NewGUID = newGUID;
			}
		}
	
		private static List<ScriptData> scriptData = new List<ScriptData>()
		{
			{ new ScriptData(typeof(USInternalCurve), 		432088449, "fb48e3a42295a4bf29460478c563720f", 		-1282336031, "fb48e3a42295a4bf29460478c563720f") },
			{ new ScriptData(typeof(USInternalKeyframe), 	-1900151445, "fb48e3a42295a4bf29460478c563720f", 	908898580, "fb48e3a42295a4bf29460478c563720f") },
			{ new ScriptData(typeof(USObserverKeyframe), 	2011729394, "fb48e3a42295a4bf29460478c563720f",	 	288373638, "fb48e3a42295a4bf29460478c563720f") },
			{ new ScriptData(typeof(USPropertyInfo), 		-487939381, "fb48e3a42295a4bf29460478c563720f", 	-647153372, "fb48e3a42295a4bf29460478c563720f") },
			{ new ScriptData(typeof(USSequencer), 			-696226374, "fb48e3a42295a4bf29460478c563720f", 	-1020437938, "fb48e3a42295a4bf29460478c563720f") },
			{ new ScriptData(typeof(USTimelineContainer),	-317571602, "fb48e3a42295a4bf29460478c563720f",	 	1240455186, "fb48e3a42295a4bf29460478c563720f") },
			{ new ScriptData(typeof(USTimelineEvent), 		-1558762618, "fb48e3a42295a4bf29460478c563720f",  	1507392053, "fb48e3a42295a4bf29460478c563720f") },
			{ new ScriptData(typeof(USTimelineObserver), 	-2097175984, "fb48e3a42295a4bf29460478c563720f",	-1305707991, "fb48e3a42295a4bf29460478c563720f") },
			{ new ScriptData(typeof(USTimelineProperty), 	701273288, "fb48e3a42295a4bf29460478c563720f",	 	-719039344, "fb48e3a42295a4bf29460478c563720f") },
			//{ new ScriptData(typeof(USTimelineAnimation), 	11500000, "e0ddde56913b1462cb37b7e68657df2e", 	874444608, "fb48e3a42295a4bf29460478c563720f") },
			//{ new ScriptData(typeof(USTimelineObjectPath), 	11500000, "d261e6133e1d94043891475e037b8e46", 	-1649978195, "fb48e3a42295a4bf29460478c563720f") },
		};
		
		[MenuItem("Window/Well Fired Development/uSequencer/Utility/Upgrade Sequences To Namespace")]
		private static void Convert()
		{
			var assetPaths = AssetDatabase.GetAllAssetPaths();
			foreach(var assetPath in assetPaths)
			{
				if(!assetPath.EndsWith(".unity") && !assetPath.EndsWith(".prefab") && !assetPath.EndsWith(".asset"))
					continue;
				
				var text = File.ReadAllText(assetPath);
				
				foreach(var data in scriptData)
				{
					var searchPattern = data.GetFullOldDescription;
					var replacePattern = data.GetFullNewDescription;
	
					if(text.Contains(searchPattern))
						text = text.Replace(searchPattern, replacePattern);
				}
	
				var applicationPath = Application.dataPath;
				applicationPath = applicationPath.Remove(applicationPath.LastIndexOf('/'));
				var fullPath = string.Format("{0}/{1}", applicationPath, assetPath);
				File.WriteAllText(fullPath, text);
			}
			
			AssetDatabase.SaveAssets();
			EditorUtility.FocusProjectWindow();
		}
	}
}