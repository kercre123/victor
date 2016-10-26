using UnityEngine;
using UnityEditor;
using Crosstales.BWF.Manager;
using Crosstales.BWF.Util;

namespace Crosstales.BWF.EditorExt
{
    /// <summary>Custom editor for the 'BadWordManager'-class.</summary>
    [CustomEditor(typeof(BadWordManager))]
    public class BadWordManagerEditor : Editor
    {

        #region Variables

        private BadWordManager script;

        private string inputText = "Martians are assholes!";
        private string outputText;

        #endregion

        #region Editor methods

        public void OnEnable()
        {
            script = (BadWordManager)target;

            if (script.isActiveAndEnabled)
            {
                BadWordManager.Load();
            }
        }

        public override void OnInspectorGUI()
        {
            DrawDefaultInspector();

            EditorHelper.SeparatorUI();

            if (script.isActiveAndEnabled)
            {
                if (BadWordManager.isReady)
                {
                    GUILayout.Label("Test-Drive", EditorStyles.boldLabel);

                    if (Helper.isEditorMode)
                    {
                        inputText = EditorGUILayout.TextField(new GUIContent("Input Text", "Text to check."), inputText);

                        EditorHelper.ReadOnlyTextField("Output Text", outputText);

                        GUILayout.Space(8);

                        GUILayout.BeginHorizontal();
                        if (GUILayout.Button(new GUIContent("Contains", "Contains any bad words?")))
                        {
                            BadWordManager.Load();
                            outputText = BadWordManager.Contains(inputText).ToString();
                        }

                        if (GUILayout.Button(new GUIContent("Get", "Get all bad words.")))
                        {
                            BadWordManager.Load();
                            outputText = string.Join(", ", BadWordManager.GetAll(inputText).ToArray());
                        }

                        if (GUILayout.Button(new GUIContent("Replace", "Check and replace all bad words.")))
                        {
                            BadWordManager.Load();
                            outputText = BadWordManager.ReplaceAll(inputText);
                        }

                        if (GUILayout.Button(new GUIContent("Mark", "Mark all bad words.")))
                        {
                            BadWordManager.Load();
                            outputText = BadWordManager.Mark(inputText, BWFManager.GetAll(inputText));
                        }
                        GUILayout.EndHorizontal();
                    }
                    else
                    {
                        GUILayout.Label("Disabled in Play-mode!");
                    }
                }
            }
            else
            {
                GUILayout.Label("Script is disabled!", EditorStyles.boldLabel);
            }
        }

        public override bool RequiresConstantRepaint()
        {
            return true;
        }

        #endregion

    }
}
// Copyright 2016 www.crosstales.com