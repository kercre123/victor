using System;
using UnityEngine;

namespace ScriptedSequences {
  public class ScriptedSequenceEditorColors : ScriptableObject {
    public Color NodeColor = Color.yellow;
    public Color NodeTextColor = Color.black;
    public Color NodeBackgroundColor = new Color(0.35f, 0.35f, 0);

    public Color ConditionColor = Color.red;
    public Color ConditionTextColor = Color.white;
    public Color ConditionBackgroundColor = new Color(0.35f, 0, 0);

    public Color ActionColor = Color.blue;
    public Color ActionTextColor = Color.white;
    public Color ActionBackgroundColor = new Color(0, 0, 0.35f);
  }
}

