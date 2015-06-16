using UnityEngine;
using UnityEditor;
using System.Collections;

namespace WellFired
{
	public static class USEditor
	{
		public static bool FoldoutLabel(Rect rect, bool isExpanded, string label, out bool wasLabelPressed)
		{
			Rect foldoutRect = rect;
			foldoutRect.width = 12.0f;
	
			Rect labelRect = rect;
			labelRect.x += 12.0f;
			labelRect.width -= 12.0f;
	
			wasLabelPressed = false;
			if(Event.current.type == EventType.MouseDown && labelRect.Contains(Event.current.mousePosition))
				wasLabelPressed = true;
	
			bool expanded = EditorGUI.Foldout(foldoutRect, isExpanded, "");
			GUI.Label(labelRect, label);
			return expanded;
		}
		
		public static bool FoldoutLabel(Rect rect, bool isExpanded, GUIContent content, out bool wasLabelPressed)
		{
			Rect foldoutRect = rect;
			foldoutRect.width = 12.0f;
	
			Rect labelRect = rect;
			labelRect.x += 12.0f;
			labelRect.width -= 12.0f;
			
			wasLabelPressed = false;
			if(Event.current.type == EventType.MouseDown && labelRect.Contains(Event.current.mousePosition))
				wasLabelPressed = true;
			
			bool expanded = EditorGUI.Foldout(foldoutRect, isExpanded, "");
			GUI.Label(labelRect, content);
			return expanded;
		}

		public static USTimelineContainer DuplicateTimelineContainer(USTimelineContainer timelineContainer, USSequencer newSequence)
		{
			var duplicateTimelineGameObject = GameObject.Instantiate(timelineContainer.gameObject) as GameObject;
			var duplicateTimelineContainer = duplicateTimelineGameObject.GetComponent<USTimelineContainer>();
			duplicateTimelineContainer.transform.parent = newSequence.transform;
			duplicateTimelineContainer.transform.name = timelineContainer.name;
			USDetachScriptableObjects.ProcessTimelineContainer(duplicateTimelineContainer);
			return duplicateTimelineContainer;
		}

		public static USTimelineBase DuplicateTimeline(USTimelineBase timeline, USTimelineContainer newTimelineContainer)
		{
			var duplicateTimelineGameObject = GameObject.Instantiate(timeline.gameObject) as GameObject;
			var duplicateTimeline = duplicateTimelineGameObject.GetComponent<USTimelineBase>();
			duplicateTimeline.transform.parent = newTimelineContainer.transform;
			duplicateTimeline.transform.name = timeline.name;
			USDetachScriptableObjects.ProcessTimeline(duplicateTimeline);
			return duplicateTimeline;
		}
	}
}