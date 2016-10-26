using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections.Generic;
using Crosstales.BWF.Filter;
using Crosstales.BWF.Model;
using Crosstales.BWF.Util;

namespace Crosstales.BWF.Demo
{
    /// <summary>Main GUI controller.</summary>
    public class GUIMain : MonoBehaviour
    {

        #region Variables

        public bool AutoTest = true;
        public bool AutoReplace = false;
        public bool Fuzzy = false;

        public float IntervalCheck = 0.5f;
        public float IntervalReplace = 0.5f;

        public InputField Text;
        public Text OutputText;

        public Text BadWordList;
        public Text BadWordCounter;

        public Text Version;

        public Toggle TestEnabled;
        public Toggle ReplaceEnabled;

        public Toggle Badword;
        public Toggle Domain;
        public Toggle Capitalization;
        public Toggle Punctuation;

        public InputField BadwordReplaceChars;
        public InputField DomainReplaceChars;
        public InputField CapsTrigger;
        public InputField PuncTrigger;

        public Toggle FuzzyEnabled;

        public Image BadWordListImage;

        public Color32 GoodColor = new Color32(0, 255, 0, 192);
        public Color32 BadColor = new Color32(255, 0, 0, 192);

        public ManagerMask BadwordManager = ManagerMask.BadWord;
        public ManagerMask DomainManager = ManagerMask.Domain;
        public ManagerMask CapsManager = ManagerMask.Capitalization;
        public ManagerMask PuncManager = ManagerMask.Punctuation;

        public List<string> Sources = new List<string>(30);

        private List<string> badWords = new List<string>();

        private float elapsedTimeCheck = 0f;
        private float elapsedTimeReplace = 0f;

        private BadWordFilter bwf;
        private DomainFilter df;
        private CapitalizationFilter cf;
        private PunctuationFilter pf;

        private bool tested = false;

        #endregion

        #region MonoBehaviour methods

        public void Start()
        {
            bwf = ((BadWordFilter)(BWFManager.Filter(ManagerMask.BadWord)));
            df = ((DomainFilter)(BWFManager.Filter(ManagerMask.Domain)));
            cf = ((CapitalizationFilter)(BWFManager.Filter(ManagerMask.Capitalization)));
            pf = ((PunctuationFilter)(BWFManager.Filter(ManagerMask.Punctuation)));

            Version.text = Constants.ASSET_NAME + " - " + Constants.ASSET_VERSION;

            if (!AutoTest)
            {
                TestEnabled.isOn = false;
            }

            if (!AutoReplace)
            {
                ReplaceEnabled.isOn = false;
            }

            if (BadwordManager != ManagerMask.BadWord)
            {
                Badword.isOn = false;
            }

            if (DomainManager != ManagerMask.Domain)
            {
                Domain.isOn = false;
            }

            if (CapsManager != ManagerMask.Capitalization)
            {
                Capitalization.isOn = false;
            }

            if (PuncManager != ManagerMask.Punctuation)
            {
                Punctuation.isOn = false;
            }

            bwf.isFuzzy = Fuzzy;
            if (!Fuzzy)
            {
                FuzzyEnabled.isOn = false;
            }

            BadwordReplaceChars.text = bwf.ReplaceCharacters;
            DomainReplaceChars.text = df.ReplaceCharacters;
            CapsTrigger.text = cf.CharacterNumber.ToString();
            PuncTrigger.text = pf.CharacterNumber.ToString();

            BadWordList.text = badWords.Count > 0 ? string.Empty : "Not tested";

            Text.text = string.Empty;
        }

        public void Update()
        {
            elapsedTimeCheck += Time.deltaTime;
            elapsedTimeReplace += Time.deltaTime;

            if (AutoTest && !AutoReplace && elapsedTimeCheck > IntervalCheck)
            {
                Test();

                elapsedTimeCheck = 0f;
            }

            if (AutoReplace && elapsedTimeReplace > IntervalReplace)
            {
                Replace();

                elapsedTimeReplace = 0f;
            }

            bwf.ReplaceCharacters = BadwordReplaceChars.text;
            df.ReplaceCharacters = DomainReplaceChars.text;

            int number;
            bool res = int.TryParse(CapsTrigger.text, out number);
            cf.CharacterNumber = res ? (number > 2 ? number : 2) : 2;
            CapsTrigger.text = cf.CharacterNumber.ToString();

            res = int.TryParse(PuncTrigger.text, out number);
            pf.CharacterNumber = res ? (number > 2 ? number : 2) : 2;
            PuncTrigger.text = pf.CharacterNumber.ToString();

            if (tested)
            {
                if (badWords.Count > 0)
                {
                    BadWordList.text = string.Join(Environment.NewLine, badWords.ToArray());
                    BadWordListImage.color = BadColor;
                }
                else
                {
                    BadWordList.text = "No bad words found";
                    BadWordListImage.color = GoodColor;
                }
            }

            BadWordCounter.text = badWords.Count.ToString();

            OutputText.text = BWFManager.Mark(Text.text, badWords);
        }

        #endregion

        #region Public methods

        public void TestChanged(bool val)
        {
            AutoTest = val;
        }

        public void ReplaceChanged(bool val)
        {
            AutoReplace = val;
        }

        public void BadwordChanged(bool val)
        {
            BadwordManager = val ? ManagerMask.BadWord : ManagerMask.None;
        }

        public void DomainChanged(bool val)
        {
            DomainManager = val ? ManagerMask.Domain : ManagerMask.None;
        }

        public void CapitalizationChanged(bool val)
        {
            CapsManager = val ? ManagerMask.Capitalization : ManagerMask.None;
        }

        public void PunctuationChanged(bool val)
        {
            PuncManager = val ? ManagerMask.Punctuation : ManagerMask.None;
        }

        public void FuzzyChanged(bool val)
        {
            bwf.isFuzzy = val;
        }

        public void FullscreenChanged(bool val)
        {
            Screen.fullScreen = val;
        }

        public void Test()
        {
            tested = true;

            if (!string.IsNullOrEmpty(Text.text))
            {

                badWords = BWFManager.GetAll(Text.text, BadwordManager | DomainManager | CapsManager | PuncManager, Sources.ToArray());

                //            if (badWords != null && badWords.Count > 0) {
                //               Text.text = MultiManager.Mark(Text.text, badWords);
                //            }
            }
        }

        public void Replace()
        {
            tested = true;

            if (!string.IsNullOrEmpty(Text.text))
            {
                //Text.text = MultiManager.Unmark(Text.text);

                Text.text = BWFManager.ReplaceAll(Text.text, BadwordManager | DomainManager | CapsManager | PuncManager, Sources.ToArray());

                badWords.Clear();
            }
        }

        public void OpenAssetURL()
        {
#if UNITY_WEBPLAYER
         Application.ExternalEval("window.open('" + Constants.ASSET_URL + "','" + Constants.ASSET_URL + "')");
#else
            Application.OpenURL(Constants.ASSET_URL);
#endif
        }

        public void OpenCTURL()
        {
#if UNITY_WEBPLAYER
         Application.ExternalEval("window.open('" + Constants.ASSET_AUTHOR_URL + "','" + Constants.ASSET_AUTHOR_URL + "')");
#else
            Application.OpenURL(Constants.ASSET_AUTHOR_URL);
#endif
        }

        public void Quit()
        {
            Application.Quit();
        }

        #endregion
    }
}
// Copyright 2015-2016 www.crosstales.com