using UnityEngine;
using UnityEditor;
using Crosstales.BWF.Manager;
using Crosstales.BWF.Util;

namespace Crosstales.BWF.EditorExt
{
    /// <summary>Custom editor for the 'PunctuationManager'-class.</summary>
    [CustomEditor(typeof(PunctuationManager))]
    public class PunctuationManagerEditor : Editor
    {

        #region Variables

        private PunctuationManager script;

        private string inputText = "Come on, test me!!!!!!";
        private string outputText;

        #endregion

        #region Editor methods

        public void OnEnable()
        {
            script = (PunctuationManager)target;
        }

        public override void OnInspectorGUI()
        {
            DrawDefaultInspector();

            EditorHelper.SeparatorUI();

            if (script.isActiveAndEnabled)
            {
                if (PunctuationManager.isReady)
                {
                    GUILayout.Label("Test-Drive", EditorStyles.boldLabel);

                    if (Helper.isEditorMode)
                    {
                        inputText = EditorGUILayout.TextField(new GUIContent("Input Text", "Text to check."), inputText);

                        EditorHelper.ReadOnlyTextField("Output Text", outputText);

                        GUILayout.Space(8);

                        GUILayout.BeginHorizontal();
                        if (GUILayout.Button(new GUIContent("Contains", "Contains any extensive punctuations?")))
                        {
                            PunctuationManager.Load();
                            outputText = PunctuationManager.Contains(inputText).ToString();
                        }

                        if (GUILayout.Button(new GUIContent("Get", "Get all extensive punctuations.")))
                        {
                            PunctuationManager.Load();
                            outputText = string.Join(", ", PunctuationManager.GetAll(inputText).ToArray());
                        }

                        if (GUILayout.Button(new GUIContent("Replace", "Check and replace all extensive punctuations.")))
                        {
                            PunctuationManager.Load();
                            outputText = PunctuationManager.ReplaceAll(inputText);
                        }

                        if (GUILayout.Button(new GUIContent("Mark", "Mark all extensive punctuations.")))
                        {
                            PunctuationManager.Load();
                            outputText = PunctuationManager.Mark(inputText, BWFManager.GetAll(inputText));
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