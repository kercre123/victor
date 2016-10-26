using UnityEngine;
using System.Diagnostics;
using System.Collections;
using Crosstales.BWF.Filter;
using Crosstales.BWF.Model;

namespace Crosstales.BWF.Test
{
    /// <summary>Base class for all tests.</summary>
    public abstract class BaseTest : MonoBehaviour
    {
        #region Variables
        //Inspector variables
        public int Iterations = 50;
        public int TextStartLength = 100;
        public int TextGrowPerIteration = 0;

        public ManagerMask[] Managers;
        public string[] TestSources;

        public string RandomChars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.?!-*";

        public char ReplaceChar = '*';

        //public bool Debugging = false;

        //private variables
        protected System.Random rd = new System.Random();

        protected Stopwatch stopWatch = new Stopwatch();

        protected int failCounter = 0;
        //protected float totalTime = 0f;

        protected static readonly string badword = "Fuuuccckkk";
        protected static readonly string noBadword = "assume";
        protected static readonly string domain = "goOgle.cOm";
        protected static readonly string email = "stEve76@goOgle.cOm";
        protected static readonly string noDomain = "my.cOmMand";
        protected static readonly string scunthorpe = "scuntHorPe";
        protected static readonly string arabicBadword = @"آنتاكلب";
        protected static readonly string globalBadword = "h!+leR";
        protected static readonly string nameBadword = "bAmbi";
        protected static readonly string emoji = "卍";

        protected BadWordFilter bwf;
        protected DomainFilter df;
        protected CapitalizationFilter cf;
        protected PunctuationFilter pf;

        private bool isFirsttime = true;
        #endregion

        #region MonoBehaviour methods

        public virtual void Update()
        {
            if (isFirsttime)
            {
                StartCoroutine(runTest());
                isFirsttime = false;
            }
        }
        #endregion

        #region Protected methods
        protected virtual IEnumerator runTest()
        {
            UnityEngine.Debug.Log("*** '" + this.name + "' started. ***");

            while (!BWFManager.isReady)
            {
                //UnityEngine.Debug.Log("Waiting");
                yield return null;
            }

            bwf = ((BadWordFilter)(BWFManager.Filter(ManagerMask.BadWord)));
            df = ((DomainFilter)(BWFManager.Filter(ManagerMask.Domain)));
            cf = ((CapitalizationFilter)(BWFManager.Filter(ManagerMask.Capitalization)));
            pf = ((PunctuationFilter)(BWFManager.Filter(ManagerMask.Punctuation)));

            //setup the managers
            bwf.ReplaceCharacters = new string(ReplaceChar, 1);
            df.ReplaceCharacters = new string(ReplaceChar, 1);

            //         UnityEngine.Debug.Log("Sources");
            //         foreach(string source in BadWordManager.GetSources()) {
            //            UnityEngine.Debug.Log("Source: " + source);
            //         }

            foreach (ManagerMask mask in Managers)
            {
                speedTest(mask);
                yield return null;

                sanityTest(mask);
                yield return null;
            }

            if (failCounter > 0)
            {
                UnityEngine.Debug.LogError("--- '" + this.name + "' ended with failures: " + failCounter + " ---");
            }
            else
            {
                UnityEngine.Debug.Log("+++ '" + this.name + "' successfully ended. +++");
            }
        }

        protected virtual string createRandomString(int stringLength)
        {
            char[] chars = new char[stringLength];

            for (int ii = 0; ii < stringLength; ii++)
            {
                chars[ii] = RandomChars[rd.Next(0, RandomChars.Length)];
            }

            return new string(chars);
        }

        protected abstract void speedTest(ManagerMask mask);

        protected abstract void sanityTest(ManagerMask mask);
        #endregion
    }
}
// Copyright 2015-2016 www.crosstales.com