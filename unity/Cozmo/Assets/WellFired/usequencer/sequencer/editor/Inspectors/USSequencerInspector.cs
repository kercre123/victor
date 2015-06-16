using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	[CustomEditor(typeof(USSequencer))]
	[CanEditMultipleObjects()]
	public class USSequencerInspector : Editor 
	{
		SerializedProperty updateOnFixedUpdate;

		private void OnEnable() 
		{
			// Setup the SerializedProperties
			updateOnFixedUpdate = serializedObject.FindProperty("updateOnFixedUpdate");
	    }
		
		public override void OnInspectorGUI()
		{
			serializedObject.Update();

			EditorGUILayout.Space();
			EditorGUILayout.Space();
			EditorGUILayout.HelpBox("Sequence Settings", MessageType.None);
			
			EditorGUILayout.PropertyField(updateOnFixedUpdate, new GUIContent("Fixed Update?"));

			EditorGUILayout.Space();
			EditorGUILayout.Space();
			EditorGUILayout.HelpBox("Utility", MessageType.None);

			var sequences = new List<USSequencer>();
			foreach(var target in targets)
			{
				var sequence = target as USSequencer;
				if(sequence == null)
					continue;
				
				sequences.Add(sequence);
			}

			if(sequences.Count == 1 && GUILayout.Button("Open and Edit Sequence"))
			{
				USWindow.OpenuSequencerWindow();
				Selection.activeObject = sequences[0];
			}

			if(sequences.Count <= 1)
			{
				EditorGUILayout.Space();
				EditorGUILayout.Space();
			}

			if(GUILayout.Button(sequences.Count > 1 ? "Duplicate Sequences" : "Duplicate Sequence"))
			{
				foreach(var sequence in sequences)
					USEditorUtility.DuplicateSequence(sequence);
			}

			if(sequences.Count == 1 && GUILayout.Button(PrefabUtility.GetPrefabObject(sequences[0].gameObject) ? "Update Prefab" : "Create Prefab"))
				USEditorUtility.CreatePrefabFrom(sequences[0], false);
			
			serializedObject.ApplyModifiedProperties();
		}
	}
}