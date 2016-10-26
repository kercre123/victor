using UnityEditor;
using UnityEngine;
using Crosstales.BWF.Util;

namespace Crosstales.BWF.EditorExt
{
    /// <summary>Editor window extension.</summary>
    public class ConfigWindow : ConfigBase
    {

        #region Variables 

        private int tab = 0;
        private int lastTab = 0;
        private string inputText = "MARTIANS are asses!!!! => mypage.com!";
        private string outputText;

        #endregion

        #region EditorWindow methods

        [MenuItem("Tools/BadWordFilter/Configuration...", false, EditorHelper.MENU_ID + 1)]
        public static void ShowWindow()
        {
            EditorWindow.GetWindow(typeof(ConfigWindow));
        }

        public void OnEnable()
        {
            titleContent = new GUIContent(Constants.ASSET_NAME);
        }

        public void OnGUI()
        {
            tab = GUILayout.Toolbar(tab, new string[] { "Configuration", "Test-Drive", "About" });

            if (tab != lastTab)
            {
                lastTab = tab;
                GUI.FocusControl(null);
            }

            if (tab == 0)
            {
                showConfiguration();

                GUILayout.Space(20);
                GUILayout.Box("", new GUILayoutOption[] { GUILayout.ExpandWidth(true), GUILayout.Height(1) });
                if (GUILayout.Button(new GUIContent("Save configuration", "Saves the configuration settings for this project.")))
                {
                    save();
                }

                GUILayout.Space(12);

                if (GUILayout.Button(new GUIContent("Reset configuration", "Resets the configuration settings for this project.")))
                {
                    if (EditorUtility.DisplayDialog("Reset configuration?", "Reset the configuration of " + Constants.ASSET_NAME + "?", "Yes", "No"))
                    {
                        Constants.Reset();
                        save();
                    }
                }
            }
            else if (tab == 1)
            {
                showTestDrive();
            }
            else
            {
                showAbout();
            }
        }

        public void OnInspectorUpdate()
        {
            if (tab == 1)
            {
                // This will only get called 10 times per second.
                Repaint();
            }
        }
        #endregion

        #region Private methods

        private void showTestDrive()
        {

            if (BWFManager.isReady)
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
                        BWFManager.Load();
                        outputText = BWFManager.Contains(inputText).ToString();
                    }

                    if (GUILayout.Button(new GUIContent("Get", "Get all bad words.")))
                    {
                        BWFManager.Load();
                        outputText = string.Join(", ", BWFManager.GetAll(inputText).ToArray());
                    }

                    if (GUILayout.Button(new GUIContent("Replace", "Check and replace all bad words.")))
                    {
                        BWFManager.Load();
                        outputText = BWFManager.ReplaceAll(inputText);
                    }

                    if (GUILayout.Button(new GUIContent("Mark", "Mark all bad words.")))
                    {
                        BWFManager.Load();
                        outputText = BWFManager.Mark(inputText, BWFManager.GetAll(inputText));
                    }
                    GUILayout.EndHorizontal();
                }
                else
                {
                    GUILayout.Label("Disabled in Play-mode!");
                }
            }
            else
            {
                EditorHelper.BWFUnavailable();
            }
        }

        #endregion
    }
}