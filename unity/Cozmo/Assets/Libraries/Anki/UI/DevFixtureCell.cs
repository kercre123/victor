using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;

public class DevFixtureCell : Anki.UI.View {
  
  [SerializeField]
  private Button _Button;
  public Button Button {
    get {
      return _Button;
    }
  }
  
  [SerializeField]
  private Text _Text;
  public Text Text {
    get {
      return _Text;
    }
  }
  
}
