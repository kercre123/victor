using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Crosstales.BWF;
using Crosstales.BWF.Manager;
using System.Threading;

public class MultiThreadTest : MonoBehaviour
{
    public string DirtyText;

    //private bool containsUnwantedWords;
    private List<string> unwantedWords = new List<string>();
    //private string cleanText;

    private string status = "Starting...";

    void Start()
    {
        Debug.Log("Test the text: " + DirtyText);
        Debug.Log("Text length: " + DirtyText.Length);

        StartCoroutine(multiTreaded());
    }

    void Update()
    {
        //Debug.Log(status + containsUnwantedWords);
        Debug.Log(status + unwantedWords.CTDump());
        //Debug.Log(status + cleanText);

    }

    private IEnumerator multiTreaded()
    {
        while (!BadWordManager.isReady)
        {
            yield return null;
        }

        //Thread worker = new Thread(() => BWFManager.ContainsMT(out containsUnwantedWords, DirtyText));
        Thread worker = new Thread(() => BWFManager.GetAllMT(out unwantedWords, DirtyText));
        //Thread worker = new Thread(() => BWFManager.ReplaceAllMT(out cleanText, DirtyText));

        //Thread worker = new Thread(() => BadWordManager.ReplaceAllMT(out CleanText, DirtyText));
        worker.Start();

        status = "Checking: ";

        do
        {
            yield return null;
        } while (worker.IsAlive);

        status = "Finished: ";
    }
}
