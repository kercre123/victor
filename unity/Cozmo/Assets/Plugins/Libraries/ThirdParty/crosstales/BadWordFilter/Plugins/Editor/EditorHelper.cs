using UnityEngine;
using UnityEditor;
using Crosstales.BWF.Util;

namespace Crosstales.BWF.EditorExt
{
    /// <summary>Editor helper class.</summary>
    public static class EditorHelper
    {
        /// <summary>Start index inside the "Tools"-menu.</summary>
        public const int MENU_ID = 1500;

        /// <summary>Shows a "BWF unavailable"-UI.</summary>
        public static void BWFUnavailable()
        {
            Color guiColor = GUI.color;

            GUILayout.Label("BWF not available!", EditorStyles.boldLabel);

            if (EditorHelper.isBWFInScene)
            {
                GUI.color = Color.red;
                GUILayout.Label("BWF not ready - please wait...");
            }
            else
            {
                GUI.color = Color.yellow;
                GUILayout.Label("Did you add the 'BWF'-prefab to the scene?");

                GUILayout.Space(8);

                if (GUILayout.Button(new GUIContent("Add BWF", "Add the BWF-prefab to the current scene.")))
                {
                    AddBWF();
                }
            }

            GUI.color = guiColor;
        }

        /// <summary>Shows a separator-UI.</summary>
        public static void SeparatorUI(int space = 20)
        {
            GUILayout.Space(space);
            GUILayout.Box("", new GUILayoutOption[] { GUILayout.ExpandWidth(true), GUILayout.Height(1) });
        }

        /// <summary>Adds the 'BWF'-prefab to the scene.</summary>
        public static void AddBWF()
        {
            if (!isBWFInScene)
            {
                PrefabUtility.InstantiatePrefab(AssetDatabase.LoadAssetAtPath("Assets" + Constants.PREFAB_PATH + "BWF.prefab", typeof(GameObject)));
            }
        }

        /// <summary>Checks if the 'BWF'-prefab is in the scene.</summary>
        /// <returns>True if the 'BWF'-prefab is in the scene.</returns>
        public static bool isBWFInScene
        {
            get
            {
                return GameObject.Find(Constants.MANAGER_SCENE_OBJECT_NAME) != null;
            }
        }

        /// <summary>Generates a read-only text field with a label.</summary>
        public static void ReadOnlyTextField(string label, string text)
        {
            EditorGUILayout.BeginHorizontal();
            {
                EditorGUILayout.LabelField(label, GUILayout.Width(EditorGUIUtility.labelWidth - 4));
                EditorGUILayout.SelectableLabel(text, EditorStyles.textField, GUILayout.Height(EditorGUIUtility.singleLineHeight));
            }
            EditorGUILayout.EndHorizontal();
        }
    }
}
// Copyright 2016 www.crosstales.com