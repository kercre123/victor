using System;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace ScriptedSequences {
  public class ScriptedSequenceManager : MonoBehaviour {

    public List<ScriptedSequence> Sequences = new List<ScriptedSequence>();

    private static ScriptedSequenceManager _instance;

    public static ScriptedSequenceManager Instance { 
      get { 
        if (_instance == null) { 
          var go = GameObject.Instantiate(Resources.Load("ScriptedSequenceManager") as GameObject);

          go.name = "ScriptedSequenceManager";
          _instance = go.GetComponent<ScriptedSequenceManager>();
          GameObject.DontDestroyOnLoad(go);
        }
        return _instance; 
      } 
    }

    private void Awake() {
      if (_instance == null) {
        _instance = this;
      }

      if (_instance != this) {
        DAS.Error(this, "Multiple ScriptedSequenceManager's initialized. Destroying newest one");
        GameObject.Destroy(gameObject);
      }

    }

    private void Start() {
      foreach (var sequence in Sequences) {
        sequence.Initialize();
      }
    }

    public void ActivateSequence(string name)
    {
      var sequence = Sequences.Find(s => s.Name == name);

      if (sequence == null) {
        DAS.Error(this, "Could not find Sequence " + name);
        return;
      }

      sequence.Enable();
    }

    public void BootrapCoroutine(IEnumerator coroutine) {
      StartCoroutine(coroutine);
    }
  }
}

