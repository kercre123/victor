using UnityEngine;
using System.Collections.Generic;
using UnityEngine.UI;

public class FeatureGatePane : MonoBehaviour {
  [SerializeField]
  private Toggle _TogglePrefab;
  [SerializeField]
  private RectTransform _ParentTransform;

  private void Start() {
    Dictionary<string, bool> map = FeatureGate.Instance.FeatureMap;
    foreach (var kvp in map) {
      Toggle clonedObj = GameObject.Instantiate(_TogglePrefab);
      clonedObj.transform.SetParent(_ParentTransform);
      clonedObj.transform.localScale = Vector3.one;
      clonedObj.isOn = kvp.Value;
      Text txt = clonedObj.GetComponentInChildren<Text>();
      if (txt != null) {
        txt.text = kvp.Key;
      }
      clonedObj.name = kvp.Key;
      clonedObj.onValueChanged.AddListener((value) => {
        SetFeatureEnabled(clonedObj.name, value);
      });
    }
  }

  private void SetFeatureEnabled(string featureName, bool isOn) {
    int count = FeatureGate.Instance.FeatureMap.Count;
    for (int i = 0; i < count; ++i) {
      Anki.Cozmo.FeatureType featureEnum = (Anki.Cozmo.FeatureType)i;
      if (featureEnum.ToString().ToLower() == featureName) {
        FeatureGate.Instance.SetFeatureEnabled(featureEnum, isOn);
      }
    }
  }

}