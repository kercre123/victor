using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class StoryboardViewController_Design : Anki.UI.ViewController {


  // Call this from an Button OnClick action in Unity Editor 
	public void performClickWithKey(string key)
  {
    PerformSegueWithIdentifier(key);
  }

}
