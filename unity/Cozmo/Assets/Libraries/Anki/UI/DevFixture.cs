using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;

public class DevFixture : Anki.UI.View {
  
  private Dictionary<string,Action> _ButtonMap = new Dictionary<string,Action>();
  // private List<Button> _Buttons = new List<Button>();

  public void AddButton(string text, Action action) {
    _ButtonMap.Add(text, action);

    GameObject obj = PrefabLoader.Instance.InstantiatePrefab("DevFixtureCell");
    DevFixtureCell cell = obj.GetComponent<DevFixtureCell>();
    cell.Text.text = text;
    cell.Button.onClick.AddListener(() => {
      action();
    });
    AddSubview(cell);
  }
  
}
