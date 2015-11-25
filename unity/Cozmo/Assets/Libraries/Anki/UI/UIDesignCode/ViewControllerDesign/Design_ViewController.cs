using UnityEngine;
using System.Collections;

public class Design_ViewController : Anki.UI.ViewController {

  public Anki.UI.ViewController modalVCPrefab;

  public void presentModalVC()
  {
    if (modalVCPrefab != null) {
      Anki.UI.ViewController modalVC = GameObject.Instantiate(modalVCPrefab) as Anki.UI.ViewController;
      this.PresentModalViewController(modalVC);
    }
  }
	
}
