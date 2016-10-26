using UnityEngine;
using UnityEditor;
using System.Threading;
using Crosstales.BWF.Util;

namespace Crosstales.BWF.EditorExt
{
    /// <summary>Base class for editor windows.</summary>
    public abstract class ConfigBase : EditorWindow
    {

        #region Variables

        protected static string updateText = UpdateCheck.TEXT_NOT_CHECKED;

        private static Thread worker;

        #endregion

        #region Protected methods

        protected static void showConfiguration()
        {
            GUILayout.Label("General Settings", EditorStyles.boldLabel);

            Constants.ASSET_PATH = EditorGUILayout.TextField(new GUIContent("Asset Path", "Path to the asset inside the Unity-project (default: " + Constants.DEFAULT_ASSET_PATH + ")."), Constants.ASSET_PATH);

            Constants.DEBUG = EditorGUILayout.Toggle(new GUIContent("Debug", "Enable or disable debug logs (default: off)."), Constants.DEBUG);
            Constants.DEBUG_BADWORDS = EditorGUILayout.Toggle(new GUIContent("Debug BadWords", "Enable or disable debug logging for BadWords (Attention: slow!, default: off)."), Constants.DEBUG_BADWORDS);
            Constants.DEBUG_DOMAINS = EditorGUILayout.Toggle(new GUIContent("Debug Domains", "Enable or disable debug logging for Domains (Attention: VERY SLOOOOOOOOWWWW!, default: off)."), Constants.DEBUG_DOMAINS);

            Constants.UPDATE_CHECK = EditorGUILayout.BeginToggleGroup(new GUIContent("Update check", "Enable or disable the update-check (default: on)."), Constants.UPDATE_CHECK);
            EditorGUI.indentLevel++;
            Constants.UPDATE_OPEN_UAS = EditorGUILayout.Toggle(new GUIContent("Open UAS-site", "Automatically opens the direct link to 'Unity AssetStore' if an update was found (default: off)."), Constants.UPDATE_OPEN_UAS);
            EditorGUI.indentLevel--;
            EditorGUILayout.EndToggleGroup();

            //Constants.DONT_DESTROY_ON_LOAD = EditorGUILayout.Toggle(new GUIContent("Don't destroy on load", "Don't destroy BWF during scene switches (default: on, off is NOT RECOMMENDED!)."), Constants.DONT_DESTROY_ON_LOAD);
            //Constants.PREFAB_PATH = EditorGUILayout.TextField(new GUIContent("Prefab path", "Path to the prefabs inside the Unity-project."), Constants.PREFAB_PATH);
            Constants.PREFAB_AUTOLOAD = EditorGUILayout.Toggle(new GUIContent("Prefab auto-load", "Enable or disable auto-loading of the prefabs to the scene (default: on)."), Constants.PREFAB_AUTOLOAD);
        }

        protected static void showAbout()
        {
            GUILayout.Label(Constants.ASSET_NAME, EditorStyles.boldLabel);
            GUILayout.Label("Version:\t" + Constants.ASSET_VERSION);

            GUILayout.Space(6);
            GUILayout.Label("Web:\t" + Constants.ASSET_AUTHOR_URL);
            GUILayout.Label("Email:\t" + Constants.ASSET_CONTACT);

            GUILayout.Space(12);
            GUILayout.Label("© 2015-2016 by " + Constants.ASSET_AUTHOR);

            EditorHelper.SeparatorUI();

            if (worker == null || (worker != null && !worker.IsAlive))
            {
                if (GUILayout.Button(new GUIContent("Check for update", "Checks for available updates of " + Constants.ASSET_NAME)))
                {
                    worker = new Thread(() => UpdateCheck.UpdateCheckForEditor(out updateText));
                    worker.Start();
                }
            }
            else
            {
                GUILayout.Label("Checking... Please wait.", EditorStyles.boldLabel);
            }

            Color fgColor = GUI.color;

            if (updateText.Equals(UpdateCheck.TEXT_NOT_CHECKED))
            {
                GUI.color = Color.cyan;
                GUILayout.Label(updateText);
            }
            else if (updateText.Equals(UpdateCheck.TEXT_NO_UPDATE))
            {
                GUI.color = Color.green;
                GUILayout.Label(updateText);
            }
            else
            {
                GUI.color = Color.yellow;
                GUILayout.Label(updateText);

                if (GUILayout.Button(new GUIContent("Download", "Opens the 'Unity AssetStore' for downloading the latest version.")))
                {
                    Application.OpenURL(Constants.ASSET_URL);
                }
            }

            GUI.color = fgColor;

            EditorHelper.SeparatorUI();

            GUILayout.BeginHorizontal();
            if (GUILayout.Button(new GUIContent("Manual", "Opens the manual.")))
            {
                Application.OpenURL(Constants.ASSET_MANUAL_URL);
            }
            if (GUILayout.Button(new GUIContent("API", "Opens the API.")))
            {
                Application.OpenURL(Constants.ASSET_API_URL);
            }
            if (GUILayout.Button(new GUIContent("Forum", "Opens the forum page.")))
            {
                Application.OpenURL(Constants.ASSET_FORUM_URL);
            }
            GUILayout.EndHorizontal();

            GUILayout.BeginHorizontal();
            if (GUILayout.Button(new GUIContent("Unity AssetStore", "Opens the 'Unity AssetStore' page.")))
            {
                Application.OpenURL(Constants.ASSET_URL);
            }

            if (GUILayout.Button(new GUIContent("Product", "Opens the product page.")))
            {
                Application.OpenURL(Constants.ASSET_CT_URL);
            }

            if (GUILayout.Button(new GUIContent(Constants.ASSET_AUTHOR, "Opens the 'crosstales' page.")))
            {
                Application.OpenURL(Constants.ASSET_AUTHOR_URL);
            }
            GUILayout.EndHorizontal();
        }

        protected static void save()
        {
            EditorPrefs.SetString(Constants.KEY_ASSET_PATH, Constants.ASSET_PATH);
            EditorPrefs.SetBool(Constants.KEY_DEBUG, Constants.DEBUG);
            EditorPrefs.SetBool(Constants.KEY_DEBUG_BADWORDS, Constants.DEBUG_BADWORDS);
            EditorPrefs.SetBool(Constants.KEY_DEBUG_DOMAINS, Constants.DEBUG_DOMAINS);
            EditorPrefs.SetBool(Constants.KEY_UPDATE_CHECK, Constants.UPDATE_CHECK);
            EditorPrefs.SetBool(Constants.KEY_UPDATE_OPEN_UAS, Constants.UPDATE_OPEN_UAS);
            //EditorPrefs.SetBool(Constants.KEY_DONT_DESTROY_ON_LOAD, Constants.DONT_DESTROY_ON_LOAD);
            EditorPrefs.SetBool(Constants.KEY_PREFAB_AUTOLOAD, Constants.PREFAB_AUTOLOAD);

            if (Constants.DEBUG)
                Debug.Log("Config data saved");
        }

        #endregion
    }
}