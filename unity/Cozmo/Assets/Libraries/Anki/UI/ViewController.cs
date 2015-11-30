using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using DG.Tweening;

namespace Anki
{
  namespace UI
  {
    [RequireComponent(typeof(View))]
    [RequireComponent(typeof(CanvasGroup))]
    public class ViewController : BaseBehaviour
    {

      public static ViewController CreateInstance(string name)
      {
        ViewController instance = new GameObject( (name == null) ? "ViewController" : name ).AddComponent<ViewController>();

        RectTransform rectTransform = instance.gameObject.AddComponent<RectTransform>();
        rectTransform.anchorMin = Vector2.zero;
        rectTransform.anchorMax = new Vector2(1.0f, 1.0f);
        rectTransform.offsetMin = Vector2.zero;
        rectTransform.offsetMax = Vector2.zero;
        rectTransform.localScale = new Vector3(1.0f, 1.0f, 1.0f);
        
        return instance;
      }

      private List<Tween> _Tweens = new List<Tween>(); 

      /// <summary>
      /// View controller's title.
      /// Note: When ViewController is in a NavigationController this title will be used for the Navigation Bar title.
      /// </summary>
      public string Title;

      /// <summary>
      /// Get View component from GameObject
      /// </summary>
      /// <value>The view.</value>
      public View View
      {
        get { return gameObject.GetComponent<View>(); }
      }

      /// <summary>
      /// The nav controller.
      /// </summary>
      public NavigationController NavigationController = null;


      /// <summary>
      /// Gets the presented view controller.
      /// </summary>
      /// <value>The presented view controller.</value>
      public ViewController PresentedViewController { get; private set; }

      /// <summary>
      /// Gets the presenting view controller.
      /// </summary>
      /// <value>The presenting view controller.</value>
      public ViewController PresentingViewController { get; private set; }


      /// <summary>
      /// Presents the modal view controller.
      /// Notes:
      /// - Modal View Controller is expected to set it's own transform.
      /// - Can only present one modal view controller at a time.
      /// </summary>
      /// <param name="viewController">View controller.</param>
      public void PresentModalViewController(ViewController viewController)
      {
        // Can Only present 1 Modal VC
        if (PresentingViewController == null) {
          // Link view hierarchy
          PresentedViewController = viewController;
          viewController.PresentingViewController = this;
          // Present ViewController on the Root transform on top of all other objects
          PresentedViewController.gameObject.SetActive(true);
          PresentedViewController.gameObject.transform.SetParent(gameObject.transform.root, false);
          PresentedViewController.gameObject.transform.SetAsLastSibling();
          DAS.Event("ui.modal", viewController.GetType().Name);
        }
        else {
          DAS.Error("ViewController", "Can NOT present Modal View Controller while already presenting another Modal View Controller");
        }
      }

      /// <summary>
      /// Dismisses the view controller.
      /// - The presenting view controller is responsible for dismissing the view controller it presented. If you call this
      /// method on the presented view controller itself, however, it automatically forwards the message to the presenting
      /// view controller.  (Stolen from Apple Docs)
      /// - Will Destroy the Presented VC after it is dismissed
      /// </summary>
      public void DismissViewController()
      {

        // Check if this is the Presenting VC by checking if it has a Presented VC
        if (PresentedViewController != null) {
          // Dismis & Destroy Pressenting VC
          DAS.Event("ui.modal.dismiss", PresentedViewController.GetType().Name);
          EventManager.Instance.VC_Dismissed(PresentedViewController);
          PresentedViewController.View.RemoveFromSuperview();
          PresentedViewController.gameObject.SetActive(false);
          Destroy(PresentedViewController.gameObject);
          PresentedViewController = null;
        }
        else if (PresentingViewController != null) {
          // Forward Call to it's pressenting View Controller
          ViewController presentingVC = this.PresentingViewController;
          PresentingViewController = null;
          presentingVC.DismissViewController();
        }
      }


      public void DismissAllChildModals()
      {
        // Maintain a list of all child view controllers, then dismiss from the end backwards
        List<ViewController> views = new List<ViewController>();
        ViewController currentVC = this;
        while (currentVC != null) {
          views.Add(currentVC);
          NavigationController navCntr = currentVC.gameObject.GetComponent<NavigationController>() as NavigationController;
          currentVC = navCntr != null ? navCntr.VisibleViewController() : null;
        }

        for (int i = views.Count - 1; i >= 0; i--) {
          views[i].DismissViewController();
        }
      }

      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Storyboard
      // These properties should only be accessed by the storyboard
      
      public Storyboard Storyboard = null;
      public string StoryboardIdentifier = null;


      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Tween Management
      public void AddTween(Tween tween){
        _Tweens.Add(tween);
      }



      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Segue

      public void PerformSegueWithIdentifier(string identifier)
      {
        if ( Storyboard != null && StoryboardIdentifier != null ) {
          Storyboard.PerformSegue(this, identifier);
        }
        else {
          DAS.Error("ViewController", "PerformSegueWithIdentifier: " + identifier + "  Storyboard or Storyboard Identifier does NOT exist");
        }
      }

      // Perform segues not described in the Storyboard
      public void PerformSegue(NavigationSegue segue)
      {
        if ( NavigationController != null ) {
          NavigationController.PerformSegueNavigation(segue);
        }
        else {
          DAS.Error("ViewController", "_PerformSegue: " + segue.Identifier + " - Navigation Controller does NOT exist");
        }
      }

      // Segue hook
      // Prepare ViewControllers before segue is performed
      public virtual void PrepareForSegue(Anki.UI.NavigationSegue segue)
      {
      }

      // Build Path
      private List<string> _GetPath() {    
        // Walk to visible ViewController
        ViewController currentVC = this;
        ViewController visibleVC = this;

        while (currentVC != null) {
          NavigationController navCntr = currentVC.gameObject.GetComponent<NavigationController>() as NavigationController;
          if (navCntr != null) {
            currentVC = navCntr.VisibleViewController();
          }
          else {
            visibleVC = currentVC;
            currentVC = null;
          }
        }
        // Generate Visible ViewController's Path
        List<string> path = new List<string>();
        currentVC = visibleVC;
        // Walk up View Controller's hierarchy
        while (currentVC != null) {     
          // Add Item to the head of the list
          path.Insert(0, currentVC.gameObject.name);

          // Parent ViewContrller
          currentVC = currentVC.NavigationController;
        }
        return path;
      }

      public List<string> GetPath() {
        return _GetPath();
      }

      public string Name() {
        return _GetPath().Last();
      }



      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Other Navigation Hooks
      // TODO JMR - Not sure this is the best approach working on other solution
      public virtual void NavigationControllerBackPressed(Anki.UI.NavigationController navigationController)
      {
        // This should not be here, it is app specific code -- JMR
        //if (SceneDirector.HasInstance) {
        //  SceneDirector.Instance.KillCurrentScene();
        //}
        if (Storyboard != null) {
          Storyboard.PerformDefaultUnwindSegue(this);
        }
        else {
          NavigationController.PopViewController();
        }
      }


      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      /// 
      /// 
      
      /// <summary>
      /// Views the did load.
      /// Note: This is called after OnLoad() is called on all of game object's components and sub-components.
      /// </summary>
      protected virtual void ViewDidLoad()
      {
      }

      /// <summary>
      /// This is called BEFORE the ViewController is added to a Navigation Controller View
      /// </summary>
      public virtual void WillTransitionIn()
      {
        EventManager.Instance.VC_WillTransitionIn(this);
      }

      /// <summary>
      /// This is called AFTER the ViewController is added to a Navigation Controller View
      /// </summary>
      public virtual void DidTransitionIn()
      {
        EventManager.Instance.VC_DidTransitionIn(this);
      }

      /// <summary>
      /// This is called BEFORE the ViewController is removed to a Navigation Controller View
      /// </summary>
      public virtual void WillTransitionOut()
      {
        // 20151019 TODO: find alternative to this ugly tight coupling
        //Anki.ApplicationServices.JavaService.RequestCloseKeyboard();
        EventManager.Instance.VC_WillTransitionOut(this);
      }

      /// <summary>
      /// This is called AFTER the ViewController is removed to a Navigation Controller View
      /// </summary>
      public virtual void DidTransitionOut()
      {
        EventManager.Instance.VC_DidTransitionOut(this);
      }
      
      
      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      /// View Life Cycle
      
      protected override void Awake()
      {
        base.Awake();
        ViewDidLoad();
      }

      protected override void OnDisable() {
        base.OnDisable();

        //Kill all tweens running on this VC
        foreach(Tween tween in _Tweens){
          tween.Kill();
        }
      }

    }
  }
}