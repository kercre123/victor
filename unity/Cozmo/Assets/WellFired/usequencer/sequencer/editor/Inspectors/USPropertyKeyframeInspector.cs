using UnityEngine;
using UnityEditor;
using System.Collections;

namespace WellFired
{
	[CustomEditor(typeof(USInternalKeyframe))]
	[CanEditMultipleObjects()]
	public class USPropertyKeyframeInspector : Editor 
	{
	    private SerializedProperty 	time;
	    private SerializedProperty 	value;
	    private SerializedProperty 	inTangent;
	    private SerializedProperty 	outTangent;
		private SerializedProperty 	brokenTangents;
		
		private SerializedObject 	keyframes;
		
		void OnEnable() 
		{	
			keyframes 		= new SerializedObject(targets);
			
	        // Setup the SerializedProperties
	        time 			= keyframes.FindProperty("time");
	        value 			= keyframes.FindProperty("value");
			inTangent 		= keyframes.FindProperty("inTangent");
			outTangent 		= keyframes.FindProperty("outTangent");
			brokenTangents 	= keyframes.FindProperty("brokenTangents");
	    }
		
		public override void OnInspectorGUI()
		{
			keyframes.Update();

			var newTime = EditorGUILayout.FloatField(new GUIContent("Fire Time"), time.floatValue);
			EditorGUILayout.PropertyField(value, new GUIContent("Value"));
			EditorGUILayout.PropertyField(inTangent, new GUIContent("In Tangent"));
			EditorGUILayout.PropertyField(outTangent, new GUIContent("Out Tangent"));
			EditorGUILayout.PropertyField(brokenTangents, new GUIContent("Broken Tangent"));

			foreach(UnityEngine.Object internalObject in targets)
			{
				if(!(internalObject is USInternalKeyframe))
					continue;
				
				USInternalKeyframe keyframe = internalObject as USInternalKeyframe;
				
				if(!keyframe.curve || keyframe.curve.CanSetKeyframeToTime(newTime))
					time.floatValue = newTime;
			}
	
			bool hasDoneSomething = false;
			bool isUndo = Event.current != null && (Event.current.type == EventType.ValidateCommand && Event.current.commandName == "UndoRedoPerformed");
			if(keyframes.ApplyModifiedProperties() || isUndo)
			{	
				foreach(UnityEngine.Object internalObject in targets)
				{
					if(!(internalObject is USInternalKeyframe))
						continue;
					
					USInternalKeyframe keyframe = internalObject as USInternalKeyframe;
					
					if(!isUndo)
						USUndoManager.PropertyChange(keyframe.curve, "Inspector");
							
					keyframe.curve.ValidateKeyframeTimes();
					keyframe.curve.BuildAnimationCurveFromInternalCurve();
					hasDoneSomething = true;
				}
			}
	
			if(hasDoneSomething)
			{
				USWindow[] windows = Resources.FindObjectsOfTypeAll<USWindow>();
				foreach(var window in windows)
					window.ExternalModification();
			}
		}
	}
}