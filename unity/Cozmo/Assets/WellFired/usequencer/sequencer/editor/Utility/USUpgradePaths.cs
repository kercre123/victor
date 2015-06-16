using UnityEngine;
using UnityEditor;
using System.Collections;

namespace WellFired
{
	public class USUpgradePaths : Editor 
	{
		public static int CurrentVersionNumber = 16;
		public static string []VersionNumbers = 
		{
			"Pre-1.1",
			"Pre-1.1",
			"Pre-1.1",
			"V1.1",
			"V1.11",
			"V1.15",
			"V1.16",
			"V1.17",
			"V1.171",
			"V1.174",
			"V1.184",
			"V1.22",
			"V1.25",
			"V1.3",
			"V1.35",
			"V1.4",
			"V1.4.1"
		};
		
		public delegate bool UpgradeFunction(USSequencer sequence);
		public static UpgradeFunction []UpgradeFunctions = 
		{
			RunUpgradePathEmpty,
			RunUpgradePathEmpty,
			RunUpgradePathEmpty,
			
			RunUpgradePathPre1_1,
			RunUpgradePath1_1To1_11,
			RunUpgradePath1_11To1_15,
			RunUpgradePath1_15To1_16,
			RunUpgradePath1_16To1_17,
			RunUpgradePath1_17To1_171,
			RunUpgradePath1_171To1_174,
			RunUpgradePath1_174To1_184,
			RunUpgradePath1_184To1_22,
			RunUpgradePath1_22To1_25,
			RunUpgradePath1_25To1_3,
			RunUpgradePath1_3To1_35,
			RunUpgradePath1_35To1_4,
			RunUpgradePath1_4To1_4_1,
		};
		
		public static bool RunUpgradePaths(USSequencer sequence)
		{
			if(sequence.Version == CurrentVersionNumber)
				return false;
			
			if(sequence.Version > CurrentVersionNumber)
			{
				Debug.LogWarning("You are using an older version of uSequencer, or you have a sequence in your scene from an older version, there is no guarantee this will work. Please upgrade uSequencer", sequence);
				return false;
			}
			
			if(CurrentVersionNumber >= VersionNumbers.Length)
			{
				Debug.LogError("We've made a mistake with the version numbers, this should never happen.");
				return false;
			}
			
			if(CurrentVersionNumber >= UpgradeFunctions.Length)
			{
				Debug.LogError("We've made a mistake with the upgrade functions, this should never happen.");
				return false;
			}
			
			if(VersionNumbers.Length != UpgradeFunctions.Length)
			{
				Debug.LogError("We've made a mistake with the version numbers or the Upgrade Functions, this should never happen.");
				return false;
			}
			
			for(; sequence.Version < CurrentVersionNumber;)
			{	
				int nextVersionNumber = sequence.Version + 1;
				
				if(!UpgradeFunctions[nextVersionNumber](sequence))
					return false;
				
				Debug.Log("Upgraded Sequence : " + sequence.name + " To " + VersionNumbers[nextVersionNumber], sequence);
			}
			
			Debug.LogWarning("Your Sequence : " + sequence.name + " has been automatically upgraded, don't forget to save your scene.", sequence);
			
			return true;
		}
		
		static bool RunUpgradePathEmpty(USSequencer sequence)
		{
			sequence.Version = 2;
			return true;
		}
		
		static bool RunUpgradePathPre1_1(USSequencer sequence)
		{
			sequence.Version = 3;
			return true;
		}
		
		static bool RunUpgradePath1_1To1_11(USSequencer sequence)
		{
			sequence.Version = 4;
			
			return true;
		}
		
		static bool RunUpgradePath1_11To1_15(USSequencer sequence)
		{
			sequence.Version = 5;
			
			USRuntimeUtility.CreateAndAttachObserver(sequence);
			
			return true;
		}
		
		static bool RunUpgradePath1_15To1_16(USSequencer sequence)
		{
			sequence.Version = 6;
			
			return true;
		}
		
		static bool RunUpgradePath1_16To1_17(USSequencer sequence)
		{
			sequence.Version = 7;
			
			foreach(USTimelineContainer timelineContainer in sequence.TimelineContainers)
			{
				foreach(USTimelineBase timelineBase in timelineContainer.Timelines)
				{
					USTimelineObserver timeline = timelineBase as USTimelineObserver;
					
					if(timeline == null)
						continue;
			
					foreach(USObserverKeyframe keyframe in timeline.observerKeyframes)
					{	
						if(keyframe.observer == null)
							keyframe.observer = timeline;
					}
				}
			}
			
			return true;
		}
		
		static bool RunUpgradePath1_17To1_171(USSequencer sequence)
		{
			sequence.Version = 8;
			
			return true;
		}
		
		static bool RunUpgradePath1_171To1_174(USSequencer sequence)
		{
			sequence.Version = 9;
			
			int count = 0;
			foreach(USTimelineContainer timelineContainer in sequence.TimelineContainers)
			{
				bool hasObserver = false;
				foreach(USTimelineBase timelineBase in timelineContainer.Timelines)
				{
					USTimelineObserver observerTimeline = timelineBase as USTimelineObserver;
					
					if(observerTimeline)
						hasObserver = true;
				}
				
				if(hasObserver)
				{
					timelineContainer.Index = -1;
				}
				else
				{
					timelineContainer.Index = count;
					count++;
				}
			}
			
			return true;
		}
		
		static bool RunUpgradePath1_174To1_184(USSequencer sequence)
		{
			sequence.Version = 10;
			
			return true;
		}
		
		static bool RunUpgradePath1_184To1_22(USSequencer sequence)
		{
			sequence.Version = 11;
			
			return true;
		}
		
		static bool RunUpgradePath1_22To1_25(USSequencer sequence)
		{
			sequence.Version = 12;
			
			return true;
		}
	
		static bool RunUpgradePath1_25To1_3(USSequencer sequence)
		{
			sequence.Version = 13;
			
			return true;
		}
		
		static bool RunUpgradePath1_3To1_35(USSequencer sequence)
		{
			sequence.Version = 14;
			
			return true;
		}
		
		static bool RunUpgradePath1_35To1_4(USSequencer sequence)
		{
			sequence.Version = 15;

			foreach(var timelineContainer in sequence.TimelineContainers)
			{
				if(timelineContainer.AffectedObject != null)
				{
					// This forces the internal string to be updated
					timelineContainer.AffectedObject = timelineContainer.AffectedObject.transform;
				}
			}
			
			return true;
		}

		static bool RunUpgradePath1_4To1_4_1(USSequencer sequence)
		{
			sequence.Version = 16;

			return true;
		}
	}
}