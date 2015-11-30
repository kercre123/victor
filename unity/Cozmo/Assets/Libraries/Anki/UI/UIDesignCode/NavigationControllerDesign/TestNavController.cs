using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki;
using Anki.UI;

public class TestNavController : NavigationController
{
  [SerializeField]
  private ViewController rootViewPrefab;

  [SerializeField]
  private ViewController Pannel2Prefab;

  [SerializeField]
  private ViewController Pannel3Prefab;

  [SerializeField]
  private ViewController Pannel4Prefab;

//  public RootViewController

  public ViewController Pannel2()
  {
    return Instantiate(Pannel2Prefab);
  }

  public ViewController Pannel3()
  {
    return Instantiate(Pannel3Prefab);
  }

  public ViewController Pannel4()
  {
    return Instantiate(Pannel4Prefab);
  }

  protected override void ViewDidLoad()
  {
    // Set Color
    base.ViewDidLoad();

    Image image = gameObject.GetComponent<Image>();
    image.color = Color.white;

    // Add first view
    PushViewController(rootViewPrefab);
  }

  public void testReplace(ViewController viewController)
  {
    ReplaceVisibleViewController(viewController);
  }

  public override void NavigationControllerDidPushViewController(ViewController viewController)
  {
    DAS.Debug("TestNavController", "NavigationControllerDidPushViewController(ViewController viewController) Title: " + viewController.Title);

    TestNavControllerBar navBar = (TestNavControllerBar) this.NavigationBar;

    if (viewController.Title == rootViewPrefab.Title) {
      navBar.NextButton.onClick.RemoveAllListeners();
      navBar.NextButton.onClick.AddListener(() => {
        PushViewController(Pannel2(), SegueAnimationTransition.AnimationType.Slide);
      });
    }
    else if (viewController.Title == Pannel2Prefab.Title) {
      navBar.NextButton.onClick.RemoveAllListeners();
      navBar.NextButton.onClick.AddListener(() => {
        PushViewController(Pannel3(), SegueAnimationTransition.AnimationType.Slide);
      });
    }
    else if (viewController.Title == Pannel3Prefab.Title) {

      navBar.NextButton.onClick.RemoveAllListeners();
      navBar.NextButton.onClick.AddListener(() => {
        PopToRootViewController(SegueAnimationTransition.AnimationType.Slide);
      });
    }
    else {
      navBar.NextButton.onClick.RemoveAllListeners();
      NavigationBar.gameObject.SetActive(false);
    }


  }

  public override void NavigationControllerDidReplaceViewController(ViewController previousViewController, ViewController newViewController)
  {
    DAS.Debug("TestNavController", "NavigationControllerDidReplaceViewController previousVCTitle: " + previousViewController.Title + " newVCTitle: " + newViewController.Title);


  }

  public override void NavigationControllerDidPopViewController(ViewController viewController)
  {
    DAS.Debug("TestNavController", "NavigationControllerDidPopViewController(ViewController viewController) Title: " + viewController.Title);

    TestNavControllerBar navBar = (TestNavControllerBar) this.NavigationBar;

    // TODO Set state for root also!

    if (viewController.Title == rootViewPrefab.Title) {
      navBar.NextButton.onClick.RemoveAllListeners();
      navBar.NextButton.onClick.AddListener(() => {
        PushViewController(Pannel2(), SegueAnimationTransition.AnimationType.Slide);
      });
    }
    else if (viewController.Title == Pannel2Prefab.Title) {
      navBar.NextButton.onClick.RemoveAllListeners();
      navBar.NextButton.onClick.AddListener(() => {
        PushViewController(Pannel2(), SegueAnimationTransition.AnimationType.Slide);
      });
    }
    else if (viewController.Title == Pannel3Prefab.Title) {
      navBar.NextButton.onClick.RemoveAllListeners();
      navBar.NextButton.onClick.AddListener(() => {
        PushViewController(Pannel3(), SegueAnimationTransition.AnimationType.Slide);
      });
    }
  }

  public override void NavigationControllerDidPopToRootViewController(ViewController rootViewController)
  {
    DAS.Debug("TestNavController", "NavigationControllerDidPopToRootViewController(ViewController viewController) Title: " + rootViewController.Title);
    TestNavControllerBar navBar = (TestNavControllerBar) this.NavigationBar;
    navBar.NextButton.onClick.RemoveAllListeners();
    navBar.NextButton.onClick.AddListener(() => {
      PushViewController(Pannel2(), SegueAnimationTransition.AnimationType.Slide);
    });
  }
}
