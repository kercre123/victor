using UnityEngine;
using UnityEditor;
using Crosstales.BWF.Util;

namespace Crosstales.BWF.EditorExt
{
    /// <summary>Loads the configuration of the asset.</summary>
    [InitializeOnLoad]
    public static class ConfigLoader
    {

        #region Constructor

        static ConfigLoader()
        {
            if (EditorPrefs.HasKey(Constants.KEY_ASSET_PATH))
            {
                Constants.ASSET_PATH = EditorPrefs.GetString(Constants.KEY_ASSET_PATH);
            }

            if (EditorPrefs.HasKey(Constants.KEY_DEBUG))
            {
                Constants.DEBUG = EditorPrefs.GetBool(Constants.KEY_DEBUG);
            }

            if (EditorPrefs.HasKey(Constants.KEY_DEBUG_BADWORDS))
            {
                Constants.DEBUG_BADWORDS = EditorPrefs.GetBool(Constants.KEY_DEBUG_BADWORDS);
            }

            if (EditorPrefs.HasKey(Constants.KEY_DEBUG_DOMAINS))
            {
                Constants.DEBUG_DOMAINS = EditorPrefs.GetBool(Constants.KEY_DEBUG_DOMAINS);
            }

            if (EditorPrefs.HasKey(Constants.KEY_UPDATE_CHECK))
            {
                Constants.UPDATE_CHECK = EditorPrefs.GetBool(Constants.KEY_UPDATE_CHECK);
            }

            if (EditorPrefs.HasKey(Constants.KEY_UPDATE_OPEN_UAS))
            {
                Constants.UPDATE_OPEN_UAS = EditorPrefs.GetBool(Constants.KEY_UPDATE_OPEN_UAS);
            }

            //if (EditorPrefs.HasKey(Constants.KEY_DONT_DESTROY_ON_LOAD))
            //{
            //    Constants.DONT_DESTROY_ON_LOAD = EditorPrefs.GetBool(Constants.KEY_DONT_DESTROY_ON_LOAD);
            //}

            if (EditorPrefs.HasKey(Constants.KEY_PREFAB_AUTOLOAD))
            {
                Constants.PREFAB_AUTOLOAD = EditorPrefs.GetBool(Constants.KEY_PREFAB_AUTOLOAD);
            }

            if (Constants.DEBUG)
                Debug.Log("Config data loaded");
        }

        #endregion
    }
}
// Copyright 2016 www.crosstales.com