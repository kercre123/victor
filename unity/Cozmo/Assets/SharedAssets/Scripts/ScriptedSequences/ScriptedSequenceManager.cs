using System;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;

namespace ScriptedSequences {
  public class ScriptedSequenceManager : MonoBehaviour {

    public List<ScriptedSequence> Sequences = new List<ScriptedSequence>();

    public List<TextAsset> SequenceTextAssets = new List<TextAsset>();

    public ScriptedSequence CurrentSequence {
      get {
        return _CurrentSequence;
      }
      private set {
        _CurrentSequence = value;
      }
    }

    private ScriptedSequence _CurrentSequence = null;

    private static ScriptedSequenceManager _instance;

    public static ScriptedSequenceManager Instance { 
      get { 
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

      foreach (var textAsset in SequenceTextAssets) {
        if (textAsset == null) {
          continue;
        }
        try {
          Sequences.Add(JsonConvert.DeserializeObject<ScriptedSequence>(textAsset.text, GlobalSerializerSettings.JsonSettings));
        }
        catch (Exception ex) {
          DAS.Error(this, "Encountered error loading ScriptedSequenceFile " + textAsset.name + ": " + ex.ToString());
        }
      }


      foreach (var sequence in Sequences) {
        sequence.Initialize();
      }
    }

    public ISimpleAsyncToken ActivateSequence(string name, bool forceReplay = false) {
      var sequence = Sequences.Find(s => s.Name == name);

      if (sequence == null) {
        string msg = "Could not find Sequence " + name;
        DAS.Error(this, msg);
        return new SimpleAsyncToken(new Exception(msg));
      }

      SimpleAsyncToken token = new SimpleAsyncToken();
      Action onSuccess = null;
      Action<Exception> onError = null;

      if (!forceReplay && !sequence.Repeatable && sequence.IsComplete) {
        DAS.Info(this, "Tried to Activate Sequence that is not repeatable and has already been played");
        token.Succeed();
        return token;
      }

      onSuccess = () => {
        token.Succeed();
        _CurrentSequence = null;
        sequence.OnComplete -= onSuccess;
        sequence.OnError -= onError;
      };
      onError = (ex) => {
        token.Fail(ex);
        _CurrentSequence = null;
        sequence.OnComplete -= onSuccess;
        sequence.OnError -= onError;
      };
      sequence.OnComplete += onSuccess;
      sequence.OnError += onError;

      _CurrentSequence = sequence;
      sequence.Enable();

      return token;
    }

    public void BootstrapCoroutine(IEnumerator coroutine) {
      StartCoroutine(coroutine);
    }

    public List<string> GetSequenceNames() {
      List<string> sequenceNames = new List<string>();
      foreach (var sequence in Sequences) {
        sequenceNames.Add(sequence.Name);
      }
      return sequenceNames;
    }
  }
}

