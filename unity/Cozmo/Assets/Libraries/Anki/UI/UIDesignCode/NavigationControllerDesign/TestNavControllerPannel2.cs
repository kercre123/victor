using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.UI;

public class TestNavControllerPannel2 : TestNavViewControllerBase {

  public Button PushButton;
  public Button ReplaceButton;

  private TestNavController testNavCntr { get { return (TestNavController)this.NavigationController; } }
	
  protected override void OnLoad()
  {
    base.OnLoad();

    PushButton.onClick.AddListener(() => {

      this.NavigationController.PushViewController(testNavCntr.Pannel3());
    });

    ReplaceButton.onClick.AddListener(() => {
      NavigationController.ReplaceVisibleViewController(testNavCntr.Pannel4());
    });

  }

  public void ShowHideNavBar()
  {
    if (NavigationController != null) {
      NavigationController.NavigationBarHidden = !NavigationController.NavigationBarHidden;
    }
  }

}
