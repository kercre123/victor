using UnityEngine;
using UnityEditor;
using System.Collections;

namespace WellFired
{
	[CustomEditor(typeof(USObserverKeyframe))]
	[CanEditMultipleObjects()]
	public class USObserverKeyframeInspector : Editor 
	{
	    SerializedProperty FireTime;
		SerializedProperty OurCamera;
		SerializedProperty TransitionType;
		SerializedProperty TransitionDuration;
		
		void OnEnable() 
		{
	        // Setup the SerializedProperties
	        FireTime = serializedObject.FindProperty("fireTime");
			OurCamera = serializedObject.FindProperty("camera");
			TransitionType = serializedObject.FindProperty("transitionType");
			TransitionDuration = serializedObject.FindProperty("transitionDuration");
	    }
		
		public override void OnInspectorGUI()
		{
			serializedObject.Update();
			
			EditorGUILayout.PropertyField(FireTime, new GUIContent("Fire Time"));
			EditorGUILayout.PropertyField(OurCamera, new GUIContent("Camera"));

			EditorGUILayout.Space();

			EditorGUILayout.HelpBox("Transitions may not preview with 100% accuracy, but your build players will always look perfect.", MessageType.Info);

			EditorGUILayout.PropertyField(TransitionType, new GUIContent("Transition Type"));

			var transitionType = (WellFired.Shared.TypeOfTransition)TransitionType.enumValueIndex;
			var transitionDuration = TransitionDuration.floatValue;
			if(transitionType != WellFired.Shared.TypeOfTransition.Cut)
			{
				EditorGUILayout.PropertyField(TransitionDuration, new GUIContent("Transition Duration"));

				if(transitionDuration <= 0.0f)
					TransitionDuration.floatValue = WellFired.Shared.TransitionHelper.DefaultTransitionTimeFor(transitionType);
			}

			if(GUILayout.Button("Select Camera"))
				Selection.activeObject = OurCamera.objectReferenceValue;
			
			serializedObject.ApplyModifiedProperties();
		}
	}
}