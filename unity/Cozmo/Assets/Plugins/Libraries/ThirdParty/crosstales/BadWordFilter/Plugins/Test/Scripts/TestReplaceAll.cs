using UnityEngine;
using Crosstales.BWF.Model;
using Crosstales.BWF.Util;

namespace Crosstales.BWF.Test
{
    /// <summary>Test for the 'ReplaceAll()' method.</summary>
    public class TestReplaceAll : BaseTest
    {
        #region Implemented methods
        protected override void speedTest(ManagerMask mask)
        {
            stopWatch.Reset();
            stopWatch.Start();

            for (int ii = 0; ii < Iterations; ii++)
            {
                BWFManager.ReplaceAll(createRandomString(TextStartLength + (TextGrowPerIteration * ii)), mask, TestSources);
            }

            stopWatch.Stop();

            //Debug.Log(totalTime + ";" + totalTime/Iterations);
            Debug.Log("## " + mask + ": " + stopWatch.ElapsedMilliseconds + ";" + ((float)stopWatch.ElapsedMilliseconds / Iterations) + " ##");
        }

        protected override void sanityTest(ManagerMask mask)
        {
            //null test
            //         Debug.Log("MASK: " + mask);
            //
            //         Debug.Log("REPLACE normal: '" + MultiManager.ReplaceAll(null, ManagerMask.All) + "'");

            if (BWFManager.ReplaceAll(null, mask).Equals(string.Empty))
            {
                if (Constants.DEBUG) Debug.Log("Nullable test passed");
            }
            else
            {
                Debug.LogError("Nullable test failed");
                failCounter++;
            }

            //empty test
            if (BWFManager.ReplaceAll(string.Empty, mask).Equals(string.Empty))
            {
                if (Constants.DEBUG) Debug.Log("Empty test passed");
            }
            else
            {
                Debug.LogError("Empty test failed");
                failCounter++;
            }

            if ((mask & ManagerMask.Domain) == ManagerMask.Domain || (mask & ManagerMask.BadWord) == ManagerMask.BadWord || (mask & ManagerMask.All) == ManagerMask.All)
            {
                //wrong 'source' test
                if (BWFManager.ReplaceAll(scunthorpe, mask, "test").Equals(scunthorpe))
                {
                    if (Constants.DEBUG) Debug.Log("Wrong 'source' test passed");
                }
                else
                {
                    Debug.LogError("Wrong 'source' test failed");
                    failCounter++;
                }

                //null for 'source' test
                if (BWFManager.ReplaceAll(scunthorpe, mask, null).Equals(scunthorpe))
                {
                    if (Constants.DEBUG) Debug.Log("Null for 'source' test passed");
                }
                else
                {
                    Debug.LogError("Null for 'source' test failed");
                    failCounter++;
                }

                //Zero-length array for 'source' test
                if (BWFManager.ReplaceAll(scunthorpe, mask, new string[0]).Equals(scunthorpe))
                {
                    if (Constants.DEBUG) Debug.Log("Zero-length array for 'source' test passed");
                }
                else
                {
                    Debug.LogError("Zero-length array for 'source' test failed");
                    failCounter++;
                }
            }

            if ((mask & ManagerMask.BadWord) == ManagerMask.BadWord || (mask & ManagerMask.All) == ManagerMask.All)
            {
                //normal bad word match test
                if (BWFManager.ReplaceAll(badword, mask).Equals(new string(ReplaceChar, badword.Length)))
                {
                    if (Constants.DEBUG) Debug.Log("Normal bad word replace test passed");
                }
                else
                {
                    Debug.LogError("Normal bad word replace test failed");
                    failCounter++;
                }

                //normal bad word non-match test
                if (BWFManager.ReplaceAll(scunthorpe, mask).Equals(scunthorpe))
                {
                    if (Constants.DEBUG) Debug.Log("Normal bad word non-replace test passed");
                }
                else
                {
                    Debug.LogError("Normal bad word non-replace word test failed");
                    failCounter++;
                }

                //bad word resource match test
                if (BWFManager.ReplaceAll(badword, mask, "english").Equals(new string(ReplaceChar, badword.Length)))
                {
                    if (Constants.DEBUG) Debug.Log("Bad word resource replace test passed");
                }
                else
                {
                    Debug.LogError("Bad word resource replace failed");
                    failCounter++;
                }

                //bad word resource non-match test
                if (BWFManager.ReplaceAll(noBadword, mask, "english").Equals(noBadword))
                {
                    if (Constants.DEBUG) Debug.Log("Bad word resource non-replace test passed");
                }
                else
                {
                    Debug.LogError("Bad word resource non-replace word test failed");
                    failCounter++;
                }

                //arabic match test
                if (BWFManager.ReplaceAll(arabicBadword, mask).Equals(new string(ReplaceChar, arabicBadword.Length)))
                {
                    if (Constants.DEBUG) Debug.Log("Arabic bad word replace test passed");
                }
                else
                {
                    Debug.LogError("Arabic bad word replace failed");
                    failCounter++;
                }

                //global match test
                if (BWFManager.ReplaceAll(globalBadword, mask).Equals(new string(ReplaceChar, globalBadword.Length)))
                {
                    if (Constants.DEBUG) Debug.Log("Global bad word replace test passed");
                }
                else
                {
                    Debug.LogError("Global bad word replace failed");
                    failCounter++;
                }

                //name match test
                if (BWFManager.ReplaceAll(nameBadword, mask).Equals(new string(ReplaceChar, nameBadword.Length)))
                {
                    if (Constants.DEBUG) Debug.Log("Name replace test passed");
                }
                else
                {
                    Debug.LogError("Name replace failed");
                    failCounter++;
                }

                //emoji match test
                if (BWFManager.ReplaceAll(emoji, mask).Equals(new string(ReplaceChar, emoji.Length)))
                {
                    if (Constants.DEBUG) Debug.Log("Emoji replace test passed");
                }
                else
                {
                    Debug.LogError("Emoji replace failed");
                    failCounter++;
                }
            }

            if ((mask & ManagerMask.Domain) == ManagerMask.Domain || (mask & ManagerMask.All) == ManagerMask.All)
            {
                //domain match test
                if (BWFManager.ReplaceAll(domain, mask).Equals(new string(ReplaceChar, domain.Length)))
                {
                    if (Constants.DEBUG) Debug.Log("Domain replace test passed");
                }
                else
                {
                    Debug.LogError("Domain replace failed");
                    failCounter++;
                }

                //email match test
                if (BWFManager.ReplaceAll(email, mask).Equals(new string(ReplaceChar, email.Length)))
                {
                    if (Constants.DEBUG) Debug.Log("Email replace test passed");
                }
                else
                {
                    Debug.LogError("Email replace failed");
                    failCounter++;
                }

                //domain non-match test
                if (BWFManager.ReplaceAll(noDomain, mask).Equals(noDomain))
                {
                    if (Constants.DEBUG) Debug.Log("Domain non-replace test passed");
                }
                else
                {
                    Debug.LogError("Domain non-replace word test failed");
                    failCounter++;
                }
            }

            if ((mask & ManagerMask.Capitalization) == ManagerMask.Capitalization)
            {
                //capital match test
                string caps = new string('A', cf.CharacterNumber);
                string noCaps = new string('A', cf.CharacterNumber - 1);

                if (BWFManager.ReplaceAll(caps, mask).Equals(caps.ToLowerInvariant()))
                {
                    if (Constants.DEBUG) Debug.Log("Capital replace test passed");
                }
                else
                {
                    Debug.LogError("Capital replace failed");
                    failCounter++;
                }

                //capital non-match test
                if (BWFManager.ReplaceAll(noCaps, mask).Equals(noCaps))
                {
                    if (Constants.DEBUG) Debug.Log("Capital non-replace test passed");
                }
                else
                {
                    Debug.LogError("Capital non-replace word test failed");
                    failCounter++;
                }
            }

            if ((mask & ManagerMask.Punctuation) == ManagerMask.Punctuation)
            {
                //punctuation match test
                string punc = new string('!', pf.CharacterNumber);
                string noPunc = new string('!', pf.CharacterNumber - 1);

                if (BWFManager.ReplaceAll(punc, mask).Equals(noPunc))
                {
                    if (Constants.DEBUG) Debug.Log("Punctuation replace test passed");
                }
                else
                {
                    Debug.LogError("Punctuation replace failed");
                    failCounter++;
                }

                //punctuation non-match test
                if (BWFManager.ReplaceAll(noPunc, mask).Equals(noPunc))
                {
                    if (Constants.DEBUG) Debug.Log("Punctuation non-replace test passed");
                }
                else
                {
                    Debug.LogError("Punctuation non-replace word test failed");
                    failCounter++;
                }
            }
        }
        #endregion
    }
}
// Copyright 2015-2016 www.crosstales.com