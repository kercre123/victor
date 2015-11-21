using System;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;

namespace ScriptedSequences {
  public class ScriptedSequenceManager : MonoBehaviour {

    public List<ScriptedSequence> Sequences = new List<ScriptedSequence>();

    public List<TextAsset> SequenceTextAssets = new List<TextAsset>();

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

    private static JsonSerializerSettings _JsonSettings;
    public static JsonSerializerSettings JsonSettings {
      get {
        if (_JsonSettings == null) {
          _JsonSettings = new JsonSerializerSettings() {
            TypeNameHandling = TypeNameHandling.Auto,
            Converters = new List<JsonConverter> {
              new UtcDateTimeConverter(),
              new Newtonsoft.Json.Converters.StringEnumConverter()
            }
          };
        }
        return _JsonSettings;
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
        try
        {
          Sequences.Add(JsonConvert.DeserializeObject<ScriptedSequence>(textAsset.text, JsonSettings));
        }
        catch(Exception ex) {
          DAS.Error(this, "Encountered error loading ScriptedSequenceFile " + textAsset.name + ": " + ex.ToString());
        }
      }


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

