using UnityEngine;
using UnityEditor;
using Crosstales.BWF.Manager;
using Crosstales.BWF.Util;

namespace Crosstales.BWF.EditorExt
{
    /// <summary>Custom editor for the 'DomainManager'-class.</summary>
    [CustomEditor(typeof(DomainManager))]
    public class DomainManagerEditor : Editor
    {

        #region Variables

        private DomainManager script;

        private string inputText = "Write me an email bwfuser@mypage.com!";
        private string outputText;

        #endregion

        #region Editor methods

        public void OnEnable()
        {
            script = (DomainManager)target;

            if (script.isActiveAndEnabled)
            {
                DomainManager.Load();
            }
        }

        public override void OnInspectorGUI()
        {
            DrawDefaultInspector();

            EditorHelper.SeparatorUI();

            if (script.isActiveAndEnabled)
            {
                if (DomainManager.isReady)
                {
                    GUILayout.Label("Test-Drive", EditorStyles.boldLabel);

                    if (Helper.isEditorMode)
                    {
                        inputText = EditorGUILayout.TextField(new GUIContent("Input Text", "Text to check."), inputText);

                        EditorHelper.ReadOnlyTextField("Output Text", outputText);

                        GUILayout.Space(8);

                        GUILayout.BeginHorizontal();
                        if (GUILayout.Button(new GUIContent("Contains", "Contains any domains?")))
                        {
                            DomainManager.Load();
                            outputText = DomainManager.Contains(inputText).ToString();
                        }

                        if (GUILayout.Button(new GUIContent("Get", "Get all domains.")))
                        {
                            DomainManager.Load();
                            outputText = string.Join(", ", DomainManager.GetAll(inputText).ToArray());
                        }

                        if (GUILayout.Button(new GUIContent("Replace", "Check and replace all domains.")))
                        {
                            DomainManager.Load();
                            outputText = DomainManager.ReplaceAll(inputText);
                        }

                        if (GUILayout.Button(new GUIContent("Mark", "Mark all domains.")))
                        {
                            DomainManager.Load();
                            outputText = DomainManager.Mark(inputText, BWFManager.GetAll(inputText));
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