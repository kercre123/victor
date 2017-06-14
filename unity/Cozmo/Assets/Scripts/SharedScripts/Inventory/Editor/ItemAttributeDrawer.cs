using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo {
  [CustomPropertyDrawer(typeof(ItemIdAttribute))]
  public class ItemAttributeDrawer : PropertyDrawer {
    private const string kItemDataConfigLocation = "Assets/AssetBundles/Shared/GameMetadata-Bundle/ItemDataConfig.asset";

    private string[] _ItemIds = null;
    // Draw the property inside the given rect
    public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
      if (_ItemIds == null) {
        _ItemIds = GetAllItemIds();
      }

      int currentOption = 0;
      string currentValue = property.stringValue;
      if (!string.IsNullOrEmpty(currentValue)) {
        for (int i = 0; i < _ItemIds.Length; i++) {
          if (_ItemIds[i] == currentValue) {
            currentOption = i;
            break;
          }
        }
      }

      int newOption = EditorGUI.Popup(position, label.text, currentOption, _ItemIds);
      string newValue = _ItemIds[newOption];

      if (newValue != currentValue) {
        property.stringValue = newValue;
        property.serializedObject.ApplyModifiedProperties();
      }
    }

    private string[] GetAllItemIds() {
      ItemDataConfig itemDataConfig = AssetDatabase.LoadAssetAtPath<ItemDataConfig>(kItemDataConfigLocation);
      List<string> allIds = new List<string>();
      allIds.AddRange(itemDataConfig.EditorGetItemIds());
      return allIds.ToArray();
    }
  }
}