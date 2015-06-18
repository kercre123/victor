using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Linq;
using System.Collections.Generic;

namespace WellFired
{
	[CustomEditor(typeof(SplineKeyframe))]
	[CanEditMultipleObjects()]
	public class SplineKeyframeInspector : Editor 
	{
		SerializedProperty position;

		private void OnEnable() 
		{
			// Setup the SerializedProperties
			position = serializedObject.FindProperty("position");
		}

		public override void OnInspectorGUI()
		{
			serializedObject.Update();

			EditorGUILayout.PropertyField(position, new GUIContent("Position"));

			using(new WellFired.Shared.GUIBeginHorizontal())
			{
				if(GUILayout.Button("Align to scene camera"))
				{
					var cameraTransform = SceneView.currentDrawingSceneView.camera.transform;
					position.vector3Value = cameraTransform.position;
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