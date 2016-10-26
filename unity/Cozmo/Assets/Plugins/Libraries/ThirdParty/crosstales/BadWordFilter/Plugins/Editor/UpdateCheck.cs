using UnityEngine;
using UnityEditor;
using System;
using System.Text;
using System.Net;
using System.IO;
using Crosstales.BWF.Util;

namespace Crosstales.BWF.EditorExt
{
    /// <summary>Checks for updates of the asset.</summary>
    [InitializeOnLoad]
    public static class UpdateCheck
    {

        #region Variables

        public const string TEXT_NOT_CHECKED = "Not checked.";
        public const string TEXT_NO_UPDATE = "No update available - you are using the latest version.";

        private static char[] splitChar = new char[] { ';' };

        #endregion

        #region Constructor

        static UpdateCheck()
        {
            if (Constants.UPDATE_CHECK)
            {
                if (Constants.DEBUG)
                    Debug.Log("Updater enabled!");

                string lastDate = EditorPrefs.GetString(Constants.KEY_UPDATE_DATE);
                string date = DateTime.Now.ToString("yyyyMMdd"); // every day
                //string date = DateTime.Now.ToString("yyyyMMddmm"); // every minute (for tests)

                if (!date.Equals(lastDate))
                {
                    if (Constants.DEBUG)
                        Debug.Log("Checking for update...");

                    EditorPrefs.SetString(Constants.KEY_UPDATE_DATE, date);

                    updateCheck();
                }
                else
                {
                    if (Constants.DEBUG)
                        Debug.Log("No update check needed.");
                }

            }
            else
            {
                if (Constants.DEBUG)
                    Debug.Log("Updater disabled!");
            }
        }

        #endregion

        #region Static methods

        public static void UpdateCheckForEditor(out string result)
        {
            string[] data = readData();

            if (isUpdateAvailable(data))
            {

                result = parseDataForEditor(data);
            }
            else
            {
                result = TEXT_NO_UPDATE;
            }
        }

        #endregion

        #region Private methods

        private static void updateCheck()
        {
            string[] data = readData();

            if (isUpdateAvailable(data))
            {

                Debug.LogWarning(parseData(data));

                if (Constants.UPDATE_OPEN_UAS)
                {
                    Application.OpenURL(Constants.ASSET_URL);
                }
            }
            else
            {
                if (Constants.DEBUG)
                    Debug.Log("Asset is up-to-date.");
            }
        }

        private static string[] readData()
        {
            string[] data = null;

            try
            {
                WebClient client = new WebClient();
                Stream stream = client.OpenRead(Constants.ASSET_UPDATE_CHECK_URL);
                StreamReader reader = new StreamReader(stream);
                String content = reader.ReadToEnd();

                foreach (string line in Helper.SplitStringToLines(content))
                {
                    if (line.StartsWith(Constants.ASSET_UID.ToString()))
                    {
                        data = line.Split(splitChar, StringSplitOptions.RemoveEmptyEntries);

                        if (data != null && data.Length >= 3) //valid record?
                        { 
                            break;
                        }
                        else
                        {
                            data = null;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.LogError("Could not load update file: " + Environment.NewLine + ex);
            }

            return data;
        }

        private static bool isUpdateAvailable(string[] data)
        {
            if (data != null)
            {
                int buildNumber;

                if (int.TryParse(data[1], out buildNumber))
                {
                    if (buildNumber > Constants.ASSET_BUILD)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        private static string parseData(string[] data)
        {
            StringBuilder sb = new StringBuilder();

            if (data != null)
            {
                sb.Append(Constants.ASSET_NAME);
                sb.Append(" - update found!");
                sb.Append(Environment.NewLine);
                sb.Append(Environment.NewLine);
                sb.Append("Your version:\t");
                sb.Append(Constants.ASSET_VERSION);
                sb.Append(Environment.NewLine);
                sb.Append("New version:\t");
                sb.Append(data[2]);
                sb.Append(Environment.NewLine);
                sb.Append(Environment.NewLine);
                sb.AppendLine("Please download the new version from the UAS:");
                sb.AppendLine(Constants.ASSET_URL);
            }

            return sb.ToString();
        }

        private static string parseDataForEditor(string[] data)
        {
            StringBuilder sb = new StringBuilder();

            if (data != null)
            {
                sb.AppendLine("Update found!");
                sb.Append(Environment.NewLine);
                sb.Append("Your version:\t");
                sb.Append(Constants.ASSET_VERSION);
                sb.Append(Environment.NewLine);
                sb.Append("New version:\t");
                sb.Append(data[2]);
                sb.Append(Environment.NewLine);
            }

            return sb.ToString();
        }

        #endregion
    }
}
// Copyright 2016 www.crosstales.com