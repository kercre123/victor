using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	[CustomEditor(typeof(USTimelineObjectPath))]
	[CanEditMultipleObjects()]
	public class USTimelineObjectPathInspector : Editor 
	{
		SerializedProperty easingType;
		SerializedProperty shakeType;
		SerializedProperty splineOrientationMode;
		SerializedProperty lookAtTarget;
		
		SerializedProperty shakeSpeedPosition;
		SerializedProperty shakeRangePosition;
		SerializedProperty shakeSpeedRotation;
		SerializedProperty shakeRangeRotation;

		private bool keyControls;

		private void OnEnable() 
		{
	        // Setup the SerializedProperties
			easingType = serializedObject.FindProperty("easingType");
			splineOrientationMode = serializedObject.FindProperty("splineOrientationMode");
			lookAtTarget = serializedObject.FindProperty("lookAtTarget");
			shakeType = serializedObject.FindProperty("shakeType");
			
			shakeSpeedPosition = serializedObject.FindProperty("shakeSpeedPosition");
			shakeRangePosition = serializedObject.FindProperty("shakeRangePosition");
			shakeSpeedRotation = serializedObject.FindProperty("shakeSpeedRotation");
			shakeRangeRotation = serializedObject.FindProperty("shakeRangeRotation");
	    }
		
		public override void OnInspectorGUI()
		{
			serializedObject.Update();
			
			var objectTimelinePaths = new List<USTimelineObjectPath>();
			foreach(var objectTarget in targets)
			{
				var objectTimelinePath = objectTarget as USTimelineObjectPath;
				if(objectTimelinePath != null)
					objectTimelinePaths.Add(objectTimelinePath);
			}

			EditorGUILayout.HelpBox(string.Format("Object Path for {0}", objectTimelinePaths.Count == 1 ? objectTimelinePaths[0].AffectedObject.name : (string.Format("{0} Objects", objectTimelinePaths.Count))), MessageType.Info);

			if(objectTimelinePaths.Count == 1)
			{
				EditorGUILayout.HelpBox(string.Format("{0} Transform Settings", objectTimelinePaths[0].AffectedObject.name), MessageType.None);

				var objectPath = target as USTimelineObjectPath;
				
				USUndoManager.BeginChangeCheck();

				var newPosition = EditorGUILayout.Vector3Field(new GUIContent("Position"), objectPath.AffectedObject.localPosition);
				var newOrientation = EditorGUILayout.Vector3Field(new GUIContent("Rotation"), objectPath.AffectedObject.localRotation.eulerAngles);

				if(USUndoManager.EndChangeCheck())
				{
					USUndoManager.PropertyChange(objectPath.AffectedObject, string.Format("Alter {0} Orientation", objectTimelinePaths[0].AffectedObject.name));
					objectPath.AffectedObject.localPosition = newPosition;
					objectPath.AffectedObject.localRotation = Quaternion.Euler(newOrientation);
				}

				if(GUILayout.Button(string.Format("Select {0}", objectTimelinePaths[0].AffectedObject.name)))
					Selection.activeGameObject = objectTimelinePaths[0].AffectedObject.gameObject;
			}
			else
			{
				EditorGUILayout.Space();
				EditorGUILayout.Space();
			}

			if(GUILayout.Button("Orient " + (objectTimelinePaths.Count > 1 ? "Objects" :  objectTimelinePaths[0].AffectedObject.name) + " To First Node"))
			{
				foreach(var objectTimelinePath in objectTimelinePaths)
				{
					USUndoManager.PropertyChange(objectTimelinePath.AffectedObject, "Orient Object");
					objectTimelinePath.SetStartingOrientation();
				}
			}
			
			EditorGUILayout.Space();
			EditorGUILayout.Space();

			EditorGUILayout.HelpBox("Path Settings", MessageType.None);
			
			EditorGUILayout.PropertyField(easingType, new GUIContent("Easing Mode"));
			EditorGUILayout.PropertyField(splineOrientationMode, new GUIContent("Orientation Mode"));

			if((SplineOrientationMode)splineOrientationMode.enumValueIndex == SplineOrientationMode.LookAtTransform)
				EditorGUILayout.PropertyField(lookAtTarget, new GUIContent("Look At Target"));
			
			EditorGUILayout.Space();
			EditorGUILayout.Space();
			EditorGUILayout.HelpBox("Shake Settings", MessageType.None);

			EditorGUILayout.PropertyField(shakeType, new GUIContent("Mode"));

			var shakeTypeEnum = (WellFired.Shared.ShakeType)shakeType.enumValueIndex;

			if(shakeTypeEnum == WellFired.Shared.ShakeType.Position || shakeTypeEnum == WellFired.Shared.ShakeType.Both)
			{
				EditorGUILayout.PropertyField(shakeSpeedPosition, new GUIContent("Speed (Position)"));
				EditorGUILayout.PropertyField(shakeRangePosition, new GUIContent("Delta (Position)"));
			}
			if(shakeTypeEnum == WellFired.Shared.ShakeType.Rotation || shakeTypeEnum == WellFired.Shared.ShakeType.Both)
			{
				EditorGUILayout.PropertyField(shakeSpeedRotation, new GUIContent("Speed (Rotation)"));
				EditorGUILayout.PropertyField(shakeRangeRotation, new GUIContent("Delta (Rotation)"));
			}

			if(objectTimelinePaths.Count == 1)
			{
				EditorGUILayout.Space();
				EditorGUILayout.Space();
				EditorGUILayout.HelpBox("Keyframe Settings", MessageType.None);
				keyControls = EditorGUILayout.Foldout(keyControls, "Edit Keyframes directly (beta)");

				if(keyControls)
				{
					foreach(var keyframe in objectTimelinePaths[0].Keyframes)
					{
						using(new WellFired.Shared.GUIBeginHorizontal())
						{
							GUILayout.Label("Keyframe");
							if(GUILayout.Button("Align to scene camera"))
							{
								USUndoManager.PropertyChange(objectTimelinePaths[0], "Align Keyframe");
								var cameraTransform = SceneView.currentDrawingSceneView.camera.transform;
								keyframe.Position = cameraTransform.position;
							}
						}
					}
				}
			}

			if(serializedObject.ApplyModifiedProperties())
			{
				USWindow[] windows = Resources.FindObjectsOfTypeAll<USWindow>();
				foreach(var window in windows)
					window.ExternalModification();
			}
		}
	}
}