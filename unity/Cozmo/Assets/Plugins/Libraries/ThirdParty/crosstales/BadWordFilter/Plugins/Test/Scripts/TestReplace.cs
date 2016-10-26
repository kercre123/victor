using UnityEngine;
using System.Collections.Generic;
using Crosstales.BWF.Model;
using Crosstales.BWF.Util;

namespace Crosstales.BWF.Test
{
    /// <summary>Test for the 'Replace' method.</summary>
    public class TestReplace : BaseTest
    {
        #region Implemented methods
        protected override void speedTest(ManagerMask mask)
        {
            List<string> badWords = new List<string>();
            badWords.Add(badword);

            stopWatch.Reset();
            stopWatch.Start();

            for (int ii = 0; ii < Iterations; ii++)
            {
                BWFManager.Replace(createRandomString(TextStartLength + (TextGrowPerIteration * ii)), badWords, mask);
            }

            stopWatch.Stop();

            //Debug.Log(totalTime + ";" + totalTime/Iterations);
            Debug.Log("## " + mask + ": " + stopWatch.ElapsedMilliseconds + ";" + ((float)stopWatch.ElapsedMilliseconds / Iterations) + " ##");
        }

        protected override void sanityTest(ManagerMask mask)
        {
            List<string> noWords = new List<string>();

            //null test
            if (BWFManager.Replace(null, noWords, mask).Equals(string.Empty))
            {
                if (Constants.DEBUG) Debug.Log("Nullable 'text test passed");
            }
            else
            {
                Debug.LogError("Nullable 'text' test failed");
                failCounter++;
            }

            if (BWFManager.Replace(scunthorpe, null, mask).Equals(scunthorpe))
            {
                if (Constants.DEBUG) Debug.Log("Nullable 'badWords test passed");
            }
            else
            {
                Debug.LogError("Nullable 'badWords' test failed");
                failCounter++;
            }

            //empty test
            if (BWFManager.Replace(string.Empty, noWords, mask).Equals(string.Empty))
            {
                if (Constants.DEBUG) Debug.Log("Empty test passed");
            }
            else
            {
                Debug.LogError("Empty test failed");
                failCounter++;
            }

            if ((mask & ManagerMask.BadWord) == ManagerMask.BadWord || (mask & ManagerMask.All) == ManagerMask.All)
            {
                List<string> badWords = new List<string>();
                badWords.Add(badword);

                //replace test
                if (BWFManager.Replace(badword, badWords, mask).Equals(new string(ReplaceChar, badword.Length)))
                {
                    if (Constants.DEBUG) Debug.Log("Bad word resource replace test passed");
                }
                else
                {
                    Debug.LogError("Bad word resource replace failed");
                    failCounter++;
                }
            }

            if ((mask & ManagerMask.Domain) == ManagerMask.Domain || (mask & ManagerMask.All) == ManagerMask.All)
            {
                List<string> domainWords = new List<string>();
                domainWords.Add(domain);

                //domain match test
                if (BWFManager.Replace(domain, domainWords, mask).Equals(new string(ReplaceChar, domain.Length)))
                {
                    if (Constants.DEBUG) Debug.Log("Domain replace test passed");
                }
                else
                {
                    Debug.LogError("Domain match failed");
                    failCounter++;
                }
            }

            if ((mask & ManagerMask.Capitalization) == ManagerMask.Capitalization)
            {
                string caps = new string('A', cf.CharacterNumber);
                List<string> capsWords = new List<string>();
                capsWords.Add(caps);

                //capital match test
                if (BWFManager.Replace(caps, capsWords, mask).Equals(caps.ToLowerInvariant()))
                {
                    if (Constants.DEBUG) Debug.Log("Capital replace test passed");
                }
                else
                {
                    Debug.LogError("Capital replace failed");
                    failCounter++;
                }
            }

            if ((mask & ManagerMask.Punctuation) == ManagerMask.Punctuation)
            {
                string punc = new string('!', pf.CharacterNumber);
                List<string> puncWords = new List<string>();
                puncWords.Add(punc);

                //punctuation match test
                if (BWFManager.Replace(punc, puncWords, mask).Equals(new string('!', pf.CharacterNumber - 1)))
                {
                    if (Constants.DEBUG) Debug.Log("Punctuation replace test passed");
                }
                else
                {
                    Debug.LogError("Punctuation replace failed");
                    failCounter++;
                }
            }
        }
        #endregion
    }
}
// Copyright 2015-2016 www.crosstales.com