using System;
using UnityEngine;
using System.Collections;

namespace ScriptedSequences {
  public class ScriptedSequenceManager : MonoBehaviour {

    private static ScriptedSequenceManager _instance;

    public static ScriptedSequenceManager Instance { 
      get { 
        if (_instance == null) { 
          var go = new GameObject();
          go.name = "ScriptedSequenceManager";
          _instance = go.AddComponent<ScriptedSequenceManager>();
          GameObject.DontDestroyOnLoad(go);
        }
        return _instance; 
      } 
    }

    private void Awake()
    {
      if (_instance == null) {
        _instance = this;
      }

      if (_instance != this) {
        DAS.Error(this, "Multiple ScriptedSequenceManager's initialized. Destroying newest one");
        GameObject.Destroy(gameObject);
      }
    }

    public void BootrapCoroutine(IEnumerator coroutine) {
      StartCoroutine(coroutine);
    }
  }
}

