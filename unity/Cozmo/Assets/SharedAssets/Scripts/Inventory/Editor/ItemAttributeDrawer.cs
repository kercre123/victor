using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo {
  [CustomPropertyDrawer(typeof(ItemIdAttribute))]
  public class ItemAttributeDrawer : PropertyDrawer {
    private const string kItemDataConfigLocation = "Assets/AssetBundles/GameMetadata-Bundle/ItemDataConfig.asset";
    private const string kHexItemListLocation = "Assets/AssetBundles/GameMetadata-Bundle/HexData/HexItemList.asset";

    private int _SelectedOption = 0;
    private string[] _ItemIds = null;
    // Draw the property inside the given rect
    public override void OnGUI(Rect position, SerializedProperty property, GUIContent label) {
      property.serializedObject.Update();

      if (_ItemIds == null) {
        _ItemIds = GetAllItemIds();

        string currentValue = property.stringValue;
        if (!string.IsNullOrEmpty(currentValue)) {
          for (int i = 0; i < _ItemIds.Length; i++) {
            if (_ItemIds[i] == currentValue) {
              _SelectedOption = i;
              break;
            }
          }
        }
      }

      _SelectedOption = EditorGUI.Popup(position, _SelectedOption, _ItemIds);
      property.stringValue = _ItemIds[_SelectedOption];

      property.serializedObject.ApplyModifiedProperties();
    }

    private string[] GetAllItemIds() {
      ItemDataConfig itemDataConfig = AssetDatabase.LoadAssetAtPath<ItemDataConfig>(kItemDataConfigLocation);
      HexItemList hexItemList = AssetDatabase.LoadAssetAtPath<HexItemList>(kHexItemListLocation);
      List<string> allIds = new List<string>();
      allIds.AddRange(itemDataConfig.EditorGetItemIds());
      allIds.AddRange(hexItemList.EditorGetPuzzlePieceIds());
      return allIds.ToArray();
    }
  }
}