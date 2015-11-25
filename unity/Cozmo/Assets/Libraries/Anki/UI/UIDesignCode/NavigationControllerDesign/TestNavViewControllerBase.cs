using UnityEngine;
using System.Collections;

public class TestNavViewControllerBase : Anki.UI.ViewController {


  protected override void OnLoad()
  {
    base.OnLoad();
    DAS.Debug("TestNavViewControllerBase.OnLoad", "VC name: " + this.gameObject.name);
  }

  protected override void ViewDidLoad()
  {
    base.ViewDidLoad();
    DAS.Debug("TestNavViewControllerBase.ViewDidLoad", "VC name: " + this.gameObject.name);
  }

  protected override void OnEnable()
  {
    base.OnEnable();
    DAS.Debug("TestNavViewControllerBase.OnEnable", "VC name: " + this.gameObject.name);
  }

  protected override void OnDisable()
  {
    base.OnDisable();
    DAS.Debug("TestNavViewControllerBase.OnDisable", "VC name: " + this.gameObject.name);
  }


  public override void WillTransitionIn()
  {
    base.WillTransitionIn();
    DAS.Debug("TestNavViewControllerBase.WillTransitionIn", "VC name: " + this.gameObject.name);
  }

  public override void WillTransitionOut()
  {
    base.WillTransitionOut();
    DAS.Debug("TestNavViewControllerBase.WillTransitionOut", "VC name: " + this.gameObject.name);
  }

  public override void DidTransitionIn()
  {
    base.DidTransitionIn();
    DAS.Debug("TestNavViewControllerBase.DidTransitionIn", "VC name: " + this.gameObject.name);
  }

  public override void DidTransitionOut()
  {
    base.DidTransitionOut();
    DAS.Debug("TestNavViewControllerBase.DidTransitionOut", "VC name: " + this.gameObject.name);
  }




	// Use this for initialization
	void Start () {
    DAS.Debug("TestNavViewControllerBase.Start", "VC name: " + this.gameObject.name);
	}
	
	
}
