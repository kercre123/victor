using UnityEngine;
using UnityEditor;
using System.Collections.Generic;
using System;
using System.Linq;

[CustomPropertyDrawer (typeof (SparkedMusicStateWrapper))]
public class SparkedMusicStateWrapperDrawer : PropertyDrawer {

  // Allows you to sort a tuple so the enums appear alphabetical
  // But we set the int.
  private class SparkedStatePairs : IComparable {
    public string OptionName;
    public int OptionValue;
    public SparkedStatePairs(string name, int val) {
      OptionName = name;
      OptionValue = val;
    }
    public int CompareTo(object obj) {
      return string.Compare(OptionName, ((SparkedStatePairs)obj).OptionName, StringComparison.Ordinal);
    }
  }
  private string [] _SortedOptions;
  private int [] _SortedValues;
  public SparkedMusicStateWrapperDrawer () {
    var type = typeof(Anki.AudioMetaData.SwitchState.Sparked);
    var options = Enum.GetNames (type);
    var values = Enum.GetValues (type).Cast<Enum> ().Select (x => (int)(uint)(object)x).ToArray ();

    List<SparkedStatePairs> sortedList = new List<SparkedStatePairs> ();
    for (int i = 0; i < options.Count (); ++i) {
      sortedList.Add(new SparkedStatePairs (options [i], values [i]));
    }
    sortedList.Sort();
    _SortedOptions = new string [sortedList.Count];
    _SortedValues = new int [sortedList.Count];
    for (int i = 0; i < sortedList.Count; ++i) {
      _SortedOptions [i] = sortedList [i].OptionName;
      _SortedValues [i] = sortedList [i].OptionValue;
    }
  }

  public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
    EditorGUI.BeginProperty(position, label, property);

    // Draw label
    position = EditorGUI.PrefixLabel(position, GUIUtility.GetControlID (FocusType.Passive), label);

    var eventProp = property.FindPropertyRelative("_Sparked");
    eventProp.intValue = EditorGUI.IntPopup(position, eventProp.intValue, _SortedOptions, _SortedValues);

    EditorGUI.EndProperty();
  }
}
