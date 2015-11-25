using UnityEngine;
using System.Collections;
using Anki.UI;

public class AlertViewDesignVC : Anki.UI.ViewController {

  
  protected override void ViewDidLoad()
  {    base.ViewDidLoad();


    AlertView alert = AlertView.CreateInstance("The Title", "The Message");
    
    alert.SetCancelButton("Cancel", cancelButtonAction);
    
    alert.ShowAlertView(this.gameObject);
  }
	

  void cancelButtonAction()
  {
    View.BackgroundImage.color = Color.white;
  }

}
