using UnityEngine;
using UnityEditor;
using System;
using Crosstales.BWF.Util;

namespace Crosstales.BWF.EditorExt
{
    /// <summary>Editor component for adding the various prefabs.</summary>
    public class BadwordMenu
    {
        [MenuItem("Tools/BadWordFilter/Add/BWF", false, EditorHelper.MENU_ID + 20)]
        private static void AddBWF()
        {
            EditorHelper.AddBWF();
        }

        [MenuItem("Tools/BadWordFilter/Add/Proxy", false, EditorHelper.MENU_ID + 40)]
        private static void AddProxy()
        {
            PrefabUtility.InstantiatePrefab(AssetDatabase.LoadAssetAtPath("Assets" + Constants.PREFAB_PATH + "Proxy.prefab", typeof(GameObject)));
        }

        [MenuItem("Tools/BadWordFilter/Help/Manual", false, EditorHelper.MENU_ID + 200)]
        private static void ShowManual()
        {
            Application.OpenURL(Constants.ASSET_MANUAL_URL);
        }

        [MenuItem("Tools/BadWordFilter/Help/API", false, EditorHelper.MENU_ID + 210)]
        private static void ShowAPI()
        {
            Application.OpenURL(Constants.ASSET_API_URL);
        }

        [MenuItem("Tools/BadWordFilter/Help/Forum", false, EditorHelper.MENU_ID + 220)]
        private static void ShowForum()
        {
            Application.OpenURL(Constants.ASSET_FORUM_URL);
        }

        [MenuItem("Tools/BadWordFilter/About/Unity AssetStore", false, EditorHelper.MENU_ID + 300)]
        private static void ShowUAS()
        {
            Application.OpenURL(Constants.ASSET_URL);
        }

        [MenuItem("Tools/BadWordFilter/About/Product", false, EditorHelper.MENU_ID + 310)]
        private static void ShowProduct()
        {
            Application.OpenURL(Constants.ASSET_CT_URL);
        }

        [MenuItem("Tools/BadWordFilter/About/" + Constants.ASSET_AUTHOR, false, EditorHelper.MENU_ID + 320)]
        private static void ShowCT()
        {
            Application.OpenURL(Constants.ASSET_AUTHOR_URL);
        }

        [MenuItem("Tools/BadWordFilter/About/Info", false, EditorHelper.MENU_ID + 340)]
        private static void ShowInfo()
        {
            EditorUtility.DisplayDialog(Constants.ASSET_NAME,
               "Version: " + Constants.ASSET_VERSION +
               Environment.NewLine +
               Environment.NewLine +
               "© 2015-2016 by " + Constants.ASSET_AUTHOR +
               Environment.NewLine +
               Environment.NewLine +
               Constants.ASSET_AUTHOR_URL +
               Environment.NewLine +
               Constants.ASSET_URL +
               Environment.NewLine, "Ok");
        }
    }
}
// Copyright 2015-2016 www.crosstales.com