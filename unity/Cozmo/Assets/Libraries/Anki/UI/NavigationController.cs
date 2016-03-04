using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

/// <summary>
/// Navigation controller.
///
/// NOTES:
/// - Don't modify This GameObject UI properties. Set a Background view.
/// </summary>
namespace Anki {
  namespace UI {
    public class NavigationController : Anki.UI.ViewController {
      // Class Consts
      public const string PushSegueIdentifier = "Push - Generated Segue";
      public const string ReplaceSegueIdentifier = "Replace - Generated Segue";
      public const string PopSegueIdentifier = "Pop - Generated Segue";
      public const string PopToRootSegueIdentifier = "PopToRoot - Generated Segue";
      public const string PopToViewControllerSegueIdentifier = "PopToViewController - Generated Segue";
      public const float DefaultSegueAnimationDuration = 0.3f;

      // Navigation Operation Types
      public enum OperationType {
        Push,
        Replace,
        Pop,
        PopToRoot,
        PopToViewController,
        Segue
      };

      /// <summary>
      /// Background View can be added to the navigation controller to provide a persistent presentation that doesn't 
      /// change when transitioning between views on the navigation stack.
      /// 
      /// Note:
      /// - The Background view's Rect Transform's is modified to fill the entire window.
      /// - If the BackgroundView is set to a new view or null the view current background view will be destroyed.
      /// </summary>
      /// <value>The background view.</value>
      public Anki.UI.View BackgroundView {
        get { return _BackgroundView; }
        set {
          // Remove current Background View
          if (_BackgroundView != null) {
            _BackgroundView.RemoveFromSuperview();
            Destroy(_BackgroundView.gameObject);
          }

          // Set new value
          _BackgroundView = value;
          if (_BackgroundView != null) {

            _BackgroundView.transform.SetParent(_BackgroundLayerView.transform, false);
            
            RectTransform rectTransform = _BackgroundView.GetComponent<RectTransform>();
            rectTransform.anchorMin = new Vector2(0.5f, 0.5f);
            rectTransform.anchorMax = new Vector2(0.5f, 0.5f);
            rectTransform.sizeDelta = new Vector2(1920.0f, 1440.0f);
            rectTransform.localScale = new Vector3(1.0f, 1.0f, 1.0f);

            //_AddFullScreenViewToParentTransform(_BackgroundView.gameObject, _BackgroundLayerView.transform);
            // TODO Need any setup?
          }
          // TODO When set to Null do I need to remove the parent from the or will setting it to null remove it?
        }
      }

      /// <summary>
      /// Foreground View can be added to the navigation controller to provide a persistent presentation that doesn't 
      /// change when transitioning between views on the navigation stack.
      /// 
      /// Notes:
      /// - The Foreground view's Rect Transform's is modified to fill the entire window.
      /// - The Navigation Bar is automatically added to the Foreground Layer, when a Foreground View is added it
      /// is placed behind the Navigation bar.
      /// - If the ForegroundView is set to a new view or null the view current foreground view will be destroyed.
      /// </summary>
      /// <value>The foreground view.</value>
      public Anki.UI.View ForegroundView {
        get { return _ForegroundView; }
        set {
          // Remove current Foreground View
          if (_ForegroundView != null) {
            _ForegroundView.RemoveFromSuperview();
            Destroy(_ForegroundView.gameObject);
          }

          _ForegroundView = value;
          if (_ForegroundView != null) {
            _AddFullScreenViewToParentTransform(_ForegroundView.gameObject, _ForegroundLayerView.transform);
            // Place the forground behind the Navigation Bar
            _ForegroundView.gameObject.transform.SetAsLastSibling();
          }
        }
      }

      /// <summary>
      /// Get or set the navigation bar.
      /// </summary>
      /// <value>The navigation bar.</value>
      public Anki.UI.NavigationBar NavigationBar {
        get { return _NavBar; }
        set {
          _NavBar = value;
          _SetUpNavBar();
        }
      }

  
      /// <summary>
      /// Get or set the height of the navigation bar.
      /// </summary>
      /// <value>The height of the navigation bar.</value>
      public float NavigationBarHeight {
        get { return _NavigationBarHeight; }
        set {
          _NavigationBarHeight = value;
          if (NavigationBar != null) {
            _LayoutNavBarTransform(NavigationBar.gameObject);
          }
        }
      }


      /// <summary>
      /// Show or hide background view.
      /// </summary>
      /// <value><c>true</c> if background view hidden; otherwise, <c>false</c>.</value>
      public bool BackgroundViewHidden {
        get { return _BackgroundLayerViewHidden; }
        set {
          _BackgroundLayerViewHidden = value;
          if (_BackgroundLayerView != null) {
            _BackgroundLayerView.gameObject.SetActive(!_BackgroundLayerViewHidden);
          }
        }
      }


      /// <summary>
      /// Show or hide the foreground view
      /// </summary>
      /// <value><c>true</c> if foreground view hidden; otherwise, <c>false</c>.</value>
      public bool ForegroundViewHidden {
        get { return _ForegroundLayerViewHidden; }
        set {
          _ForegroundLayerViewHidden = value;
          if (_ForegroundLayerView != null) {
            _ForegroundLayerView.gameObject.SetActive(!_ForegroundLayerViewHidden);
          }
        }
      }


      /// <summary>
      /// Show or hide the Navigation Bar
      /// </summary>
      /// <value><c>true</c> if navigation bar hidden; otherwise, <c>false</c>.</value>
      public bool NavigationBarHidden {
        get { return _NavigationBarHidden; }
        set {
          _NavigationBarHidden = value;
          if (_NavBar != null) {
            _NavBar.gameObject.SetActive(!_NavigationBarHidden);
          }
        }
      }


      /// <summary>
      /// Occurs when on navigation transition.
      /// ViewController: Destination ViewController
      /// float: Durration of the transition in seconds
      /// </summary>
      public event Action<Anki.UI.ViewController, float> OnNavigationTransition;

      /// <summary>
      /// Event sent immediately before performing a NavigationSegue
      /// to transition between ViewControllers.
      /// </summary>
      public event Action<Anki.UI.NavigationSegue> OnNavigationSegue;

      // Allow Root View to be set in Unity Editor
      // TODO: Create a way to access this in code
      // TODO: Deprecated
      [SerializeField]
      public ViewController
        _RootViewController;

      /// <summary>
      /// Initializes a new instance of the <see cref="Anki.UI.NavigationController"/> class.
      /// </summary>
      public NavigationController() {
      }


      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Segue based Navigation

      
      /// <summary>
      /// Performs segue navigation.
      /// Notes:
      /// - This should only be called by a ViewController with a Segue.
      /// - Segues transitions do NOT use the Stack they perform a replace operation
      /// </summary>
      /// <param name="segue">Segue.</param>
      public void PerformSegueNavigation(Anki.UI.NavigationSegue segue) {
        // Check that the segue is valid.  Source VC is the same as the Visible VC
        if (this.VisibleViewController() == segue.SourceViewController) {
          // Prep NavController
          this._PrepareForSegueNavigation(segue);
          // Perform segue
          this._PerformSegue(segue);
        }
        else {
          DAS.Error("NavigationController", "this.VisibleViewController() != segue.SourceViewController");
        }
      }


      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Stack based Navigation

      /// <summary>
      /// Initializes a new instance of the <see cref="Anki.UI.NavigationController"/> class.
      /// </summary>
      /// <param name="RootView">Root view.</param>
      public NavigationController(Anki.UI.ViewController RootView) : this() {
        this.PushViewController(RootView);
      }


      // Stack operations
      /// <summary>
      /// Pushs the view.
      /// </summary>
      /// <param name="viewController">View controller.</param>
      public void PushViewController(Anki.UI.ViewController viewController,
                                     SegueAnimationTransition.AnimationType animationType = SegueAnimationTransition.AnimationType.Fade,
                                     float animationDuration = DefaultSegueAnimationDuration,
                                     string customSegueIdentifier = null) {
        bool success = _AttemptNavigationOperation(OperationType.Push, viewController, animationType, animationDuration, customSegueIdentifier);

        if (!success) {
          // This is the Root VC, just add it to the NavController don't need a segue
          this._AddRootViewController(viewController);
        }
      }


      /// <summary>
      /// Replaces the visible view controller.
      /// Note:
      /// - Previous Visible View Controller is destroyed.
      /// </summary>
      /// <param name="viewController">View controller.</param>
      public void ReplaceVisibleViewController(Anki.UI.ViewController viewController,
                                               SegueAnimationTransition.AnimationType animationType = SegueAnimationTransition.AnimationType.Fade,
                                               float animationDuration = DefaultSegueAnimationDuration,
                                               string customSegueIdentifier = null) {
        bool success = _AttemptNavigationOperation(OperationType.Replace, viewController, animationType, animationDuration, customSegueIdentifier);
        if (!success) {
          DAS.Warn("NavigationController.ReplaceVisibleViewController", "Can NOT Replace view");
        }
      }


      /// <summary>
      /// Pops the top view controller.
      /// Note:
      /// - The view is destroyed when it is popped of the Navigation Stack. If information is needed from that view
      /// it must be collected before this method is called.
      /// </summary>
      public void PopViewController(SegueAnimationTransition.AnimationType animationType = SegueAnimationTransition.AnimationType.Fade,
                                    float animationDuration = DefaultSegueAnimationDuration,
                                    string customSegueIdentifier = null) {
        bool success = _AttemptNavigationOperation(OperationType.Pop, null, animationType, animationDuration, customSegueIdentifier);
        if (!success) {
          DAS.Warn("NavigationController.PopViewController", "Can NOT Pop View");
        }
      }


      /// <summary>
      /// Pop to root ViewController.
      /// Note:
      /// - The views are destroyed when they are popped of the Navigation Stack. If information is needed from that view
      /// it must be collected before this method is called.
      /// </summary>
      public void PopToRootViewController(SegueAnimationTransition.AnimationType animationType = SegueAnimationTransition.AnimationType.Fade,
                                          float animationDuration = DefaultSegueAnimationDuration,
                                          string customSegueIdentifier = null) {
        bool success = _AttemptNavigationOperation(OperationType.PopToRoot, null, animationType, animationDuration, customSegueIdentifier);
        if (!success) {
          DAS.Warn("NavigationController.PopToRootViewController", "Can NOT Pop to root");
        }
      }


      /// <summary>
      /// Pops to view.
      /// Note:
      /// - Pop to specific ViewController instance.
      /// </summary>
      /// <param name="viewController">View controller.</param>
      public void PopToViewController(Anki.UI.ViewController viewController,
                                      SegueAnimationTransition.AnimationType animationType = SegueAnimationTransition.AnimationType.Fade,
                                      float animationDuration = DefaultSegueAnimationDuration,
                                      string customSegueIdentifier = null) {
        bool success = _AttemptNavigationOperation(OperationType.PopToViewController, viewController, animationType, animationDuration, customSegueIdentifier);
        if (!success) {
          DAS.Warn("NavigationController.PopToViewController", "Can NOT Pop to view");
        }
      }


      /// <summary>
      /// Returns the current, visible view controller
      /// </summary>
      /// <returns>The view controller.</returns>
      public Anki.UI.ViewController VisibleViewController() {
        if (_NavStackLayerViewControllers.Count > 0) {
          return _NavStackLayerViewControllers.Last();
        }
        DAS.Warn("NavigationController", "No Visible View Controller");
        return null;
      }


      /// <summary>
      /// Navigation Stack Views at index.
      /// </summary>
      /// <returns>The controller at index.</returns>
      /// <param name="index">Index.</param>
      public Anki.UI.ViewController ViewControllerAtIndex(int index) {
        if (index < _NavStackLayerViewControllers.Count) {
          return _NavStackLayerViewControllers[index];
        }
        return null;
      }

      /// <summary>
      /// Count of Navigation Stack Views
      /// </summary>
      /// <returns>The controller count.</returns>
      public int ViewControllerCount() {
        return _NavStackLayerViewControllers.Count;
      }


      // TODO: Intergrate Asset Manager
      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // HACK Temp solution to push prefab VCs onto nav stack
      // FIXME This Must Die!!!!!


      public List<Anki.UI.ViewController> NavigationStackPrefabList;

      public virtual ViewController PrefabInstanceAtStackPrefabListIndex(int index) {
        ViewController vc = null;
        if (index < NavigationStackPrefabList.Count) {
          Anki.UI.ViewController avc = NavigationStackPrefabList[index];
          if (avc != null) {
            vc = (ViewController)GameObject.Instantiate(avc);
          }
        }

        return vc;
      }

      public virtual void PushPrefabAtIndex(int index) {
        ViewController vc = PrefabInstanceAtStackPrefabListIndex(index);
        if (vc != null) {
          PushViewController(vc);
        }
      }


      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Action Callback Hooks

      // TODO Add will/did Perform Segue call back

      /// <summary>
      /// Navigations the controller did push.
      /// </summary>
      /// <param name="vc">Vc.</param>
      public virtual void NavigationControllerDidPushViewController(Anki.UI.ViewController viewController) {
      }

      /// <summary>
      /// Navigations the controller did replace view.
      /// </summary>
      /// <param name="previousView">Previous view.</param>
      /// <param name="newView">New view.</param>
      public virtual void NavigationControllerDidReplaceViewController(Anki.UI.ViewController sourceViewController, Anki.UI.ViewController destinationViewController) {
      }

      /// <summary>
      /// Navigations the controller did pop.
      /// </summary>
      /// <param name="vc">Vc.</param>
      public virtual void NavigationControllerDidPopViewController(Anki.UI.ViewController viewController) {
      }

      /// <summary>
      /// Navigations the controller did pop to root.
      /// </summary>
      public virtual void NavigationControllerDidPopToRootViewController(Anki.UI.ViewController rootViewController) {
      }

      /// <summary>
      /// Navigations the controller did pop to view.
      /// </summary>
      /// <param name="view">View.</param>
      public virtual void NavigationControllerDidPopToViewController(Anki.UI.ViewController viewController) {
      }

      /// <summary>
      /// Navigations the controller did perfrom segue.
      /// </summary>
      /// <param name="segueIdentifier">Segue identifier.</param>
      /// <param name="sourceViewController">Source view controller.</param>
      /// <param name="destinationViewController">Destination view controller.</param>
      public virtual void NavigationControllerDidPerfromSegue(string segueIdentifier, Anki.UI.ViewController sourceViewController, Anki.UI.ViewController destinationViewController) {
      }


      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Private Vars
      [SerializeField]
      private View
        _BackgroundView;
      [SerializeField]
      private View
        _ForegroundView;
      [SerializeField]
      private NavigationBar
        _NavBar;

      // Default Navigation Bar Height
      [SerializeField]
      private float
        _NavigationBarHeight = 120.0f;

      // TODO: Should this be public?
      protected List<Anki.UI.ViewController> _NavStackLayerViewControllers = new List<Anki.UI.ViewController>();
      
      // View Layers
      private View _BackgroundLayerView;
      private View _NavStackLayerView;
      private View _ForegroundLayerView;
      private bool _BackgroundLayerViewHidden = false;
      private bool _ForegroundLayerViewHidden = false;
      private bool _NavigationBarHidden = false;

      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Private LifeCycle Methods

      protected override void ViewDidLoad() {
        DAS.Debug("NavigationController", "ViewDidLoad() - " + this.name);

        base.ViewDidLoad();
        _BackgroundLayerView = View.CreateInstance("Background Layer");
        _NavStackLayerView = View.CreateInstance("NavStack Layer");
        _ForegroundLayerView = View.CreateInstance("Foreground Layer");

        View.AddSubview(_BackgroundLayerView);
        View.AddSubview(_NavStackLayerView);
        View.AddSubview(_ForegroundLayerView);

        _SetupParentLayer(gameObject);

        if (_NavBar != null) {
          _NavBar = (NavigationBar)GameObject.Instantiate(_NavBar);
          _SetUpNavBar();
        }

        // Load Editors Root View
        if (_RootViewController != null) {
          // Note: Always create a new instance (Clone)
          ViewController vc = (ViewController)GameObject.Instantiate(_RootViewController);
          PushViewController(vc);
        }

        // Load Editors Foreground View
        if (_ForegroundView != null) {
          // Note always create a new instance (Clone)
          View editorForegroundView = (View)Instantiate(_ForegroundView);
          _ForegroundView = null;
          ForegroundView = editorForegroundView;
        }

        // Load Editors Background View
        if (_BackgroundView != null) {
          // Note always create a new instance (Clone)
          View editorBackgroundView = (View)Instantiate(_BackgroundView);
          _BackgroundView = null;
          BackgroundView = editorBackgroundView;
        }

      }

        
      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Private Helper Methods
     
      private void _AddFullScreenViewToParentTransform(GameObject vcGameObject, Transform parentView) {
        vcGameObject.transform.SetParent(parentView, false);
        this._PositionFullScreenView(vcGameObject);
      }

      private void _PositionFullScreenView(GameObject vcGameObject) {
        RectTransform rectTransform = vcGameObject.GetComponent<RectTransform>();
        rectTransform.anchorMin = Vector2.zero;
        rectTransform.anchorMax = new Vector2(1.0f, 1.0f);
        rectTransform.offsetMin = Vector2.zero;
        rectTransform.offsetMax = Vector2.zero;
        rectTransform.localScale = new Vector3(1.0f, 1.0f, 1.0f);
      }

      private void _SetupParentLayer(GameObject layerGameObject) {
        Image image = layerGameObject.GetComponent<Image>();
        if (image != null) {
          image.color = Color.clear;
        }
      }


      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Private Manage Navigation Stack

      private void _AddRootViewController(ViewController viewController) {
        // Update Nav Bar, show back button if this is not the 1st view in the stack
        _UpdateNavBar(viewController, false);
        // Add to View
        _AddViewControllerToNavigationViewLayer(viewController);
        // Store View controller
        _NavStackLayerViewControllers.Add(viewController);
        
        // Call Action
        if (OnNavigationTransition != null) {
          OnNavigationTransition(viewController, 0);
        }

        // TODO: Do I still need to call prepareForSegue() ??
        
        NavigationControllerDidPushViewController(viewController);
      }


      private void _PrepareForSegueNavigation(NavigationSegue segue) {
        // Nav Stack Book keeping
        this._NavStackLayerViewControllers.RemoveAt(this._NavStackLayerViewControllers.Count - 1);
        this._NavStackLayerViewControllers.Add(segue.DestinationViewController);

        // Calling SetActive on the DestinationVC will cause callbacks to be registered.
        // Unhook them on the SourceView now to avoid push actions in response to events.
        segue.SourceViewController.DisableCallbacks();

        segue.DestinationViewController.NavigationController = this;
        segue.DestinationViewController.gameObject.SetActive(true);
        this._NavStackLayerView.AddSubview(segue.DestinationViewController.View);

        if (segue.Direction != NavigationSegueDirection.Forward) {
          // Unwind Segue, we want the destination view behind the source view
          this._NavStackLayerView.SendSubviewToBack(segue.DestinationViewController.View);
        }
        
        // Setup Navigation Bar
        this._UpdateNavBar(segue.DestinationViewController, true);
        
        // If SourceVC / DestinationVC is a NavigationController prepare Navigation Bar
        this._PrepareSourceVCNavigationController(segue.SourceViewController);
        this._PrepareDestinationVCNavigationController(segue.DestinationViewController);

        // Perform ViewController Callbacks
        segue.SourceViewController.PrepareForSegue(segue);
        segue.DestinationViewController.WillTransitionIn();
        segue.SourceViewController.WillTransitionOut();
      }


      private bool _AttemptNavigationOperation(OperationType operationType,
                                               Anki.UI.ViewController viewController,
                                               SegueAnimationTransition.AnimationType animationType,
                                               float animationDuration,
                                               string segueIdentifier) {
        ViewController sourceVC = null;
        ViewController destinationVC = null;
        bool success = this._PrepareForTransition(operationType,
                         viewController,
                         out sourceVC,
                         out destinationVC);

        if (success) {
          // Create Push Segue
          NavigationSegue segue = this._GenerateSegueForOperation(operationType, sourceVC, destinationVC, animationType, animationDuration, segueIdentifier);
          
          if (segue != null) {
          
            // Perform ViewController Callbacks
            segue.SourceViewController.PrepareForSegue(segue);
            segue.DestinationViewController.WillTransitionIn();
            segue.SourceViewController.WillTransitionOut();

            this._PerformSegue(segue);
          }
          else {
            DAS.Error("NavigationController._AttemptNavigationOperation", "Segue is NULL");
          }
        }
        return success;
      }

      private void _AddViewControllerToNavigationViewLayer(ViewController viewController) {
        viewController.NavigationController = this;
        
        viewController.WillTransitionIn();
        
        // If DestinationVC is a NavigationController prepare Navigation Bar
        _PrepareDestinationVCNavigationController(viewController);
        
        viewController.gameObject.SetActive(true);
        _NavStackLayerView.AddSubview(viewController.View);
        
        viewController.DidTransitionIn();       
      }

      
      /// <summary>
      /// Pop to a specific view controller.
      /// Note:
      /// Views are destroyed as they are popped off the navigation stack.
      /// </summary>
      /// <param name="viewController">View controller.</param>
      // TODO Rename Method!!
      private void _PopToViewController(ViewController toViewController) {
        ViewController previousViewController = this._NavStackLayerViewControllers.Last();
        
        // Remove VC from stack until the top view matches the new VC or Root VC
        int stackCount = this._NavStackLayerViewControllers.Count;
        ViewController lastView = previousViewController;
        while (stackCount > 1 && lastView != toViewController) {
          // Clear last view
          this._NavStackLayerViewControllers.RemoveAt(stackCount - 1);
          
          if (lastView != previousViewController) {
            // If SourceVC is a NavigationController prepare Navigation Bar
            this._PrepareSourceVCNavigationController(lastView);
            
            Destroy(lastView.gameObject);
          }
          
          // Update 
          stackCount = this._NavStackLayerViewControllers.Count;
          lastView = this._NavStackLayerViewControllers.Last();
        }
      }
      
      
      // Setup Navigation Bar on Source View Controller if it is a Navigation Controller sub-class
      // As well as, prepare this navigation controller's Navigation Bar
      private void _PrepareSourceVCNavigationController(ViewController sourceViewController) {
        // If SourceVC is a NavigationController prepare Navigation Bar
        if (sourceViewController is NavigationController) {
          
          // Reset stack of NavCntrs
          NavigationController aNavCntr = (NavigationController)sourceViewController;
          while (aNavCntr != null && aNavCntr != this) {
            // Reset & Hide Navigation Bar
            aNavCntr._SetUpNavBar();
            aNavCntr.NavigationBarHidden = true;
            
            // Get next child navigation controllers 
            ViewController visibleVC = aNavCntr.VisibleViewController();
            if (visibleVC != null) {
              if (visibleVC is NavigationController) {
                aNavCntr = (NavigationController)visibleVC;
              }
              else {
                aNavCntr = null;
              }
            }
          }
          
          // Set parent Navigatoin Bar hidden state
          if (this._NavBar != null) {
            this._NavBar.gameObject.SetActive(!_NavigationBarHidden);
          }
        }
      }
      
      // Setup Navigation Bar on Destination View Controller if it is a Navigation Controller sub-class
      // As well as, prepare this navigation controller's Navigation Bar
      private void _PrepareDestinationVCNavigationController(ViewController destinationViewController) {
        // If DestinationVC is a NavigationController prepare Navigation Bar
        if (destinationViewController is NavigationController) {
          // Load Inital Storyboard Node
          Storyboard subSb = destinationViewController.GetComponent<Storyboard>();
          if (subSb != null) {
            subSb.LoadInitialNode();
          }
          
          // Add sub NavigationController's Navigation Bar to parent Navigation Controller
          if (((NavigationController)destinationViewController).NavigationBar != null) {
            // Find root navigation controller 
            NavigationController rootNavCntr = this;
            NavigationController parentNavCntr = this.NavigationController;
            while (parentNavCntr != null) {
              rootNavCntr = parentNavCntr;
              parentNavCntr = rootNavCntr.NavigationController;
            }
            ((NavigationController)destinationViewController).NavigationBar.transform.SetParent(rootNavCntr._ForegroundLayerView.transform, false);
          }
          
          // Hide parent Navigation Bar
          if (this._NavBar != null) {
            this._NavBar.gameObject.SetActive(false);
          }
        }
      }


      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Private Perform Navigation Methods

      // Return True if segue can be perfromed
      private bool _PrepareForTransition(OperationType operationType, ViewController viewController, out ViewController sourceViewController, out ViewController destinationViewController) {
        bool success = false;
        bool showBackButton = false;
        ViewController sourceVC = null;
        ViewController destinationVC = null;

        // Nav Stack Book keeping
        switch (operationType) {
          
        case OperationType.Push:
          {
            if (this._NavStackLayerViewControllers.Count > 0) {
              // Add destination to stack
              showBackButton = (_NavStackLayerViewControllers.Count > 0);
              sourceVC = this._NavStackLayerViewControllers.Last();
              destinationVC = viewController;
              this._NavStackLayerViewControllers.Add(destinationVC);
              this._NavStackLayerView.AddSubview(destinationVC.View);
              success = true;
            }
          }
          break;
            
        case OperationType.Replace:
          {
            if (this._NavStackLayerViewControllers.Count > 0) {
              // Remove Top View
              sourceVC = this._NavStackLayerViewControllers.Last();
              this._NavStackLayerViewControllers.RemoveAt(this._NavStackLayerViewControllers.Count - 1);
              // Add destination to stack
              destinationVC = viewController;
              this._NavStackLayerViewControllers.Add(destinationVC);
              this._NavStackLayerView.AddSubview(destinationVC.View);
              showBackButton = (_NavStackLayerViewControllers.Count > 1);
              success = true;
            }
          }
          break;
            
        case OperationType.Pop:
          {
            if (this._NavStackLayerViewControllers.Count > 1) {
              // Remove Top View
              sourceVC = this._NavStackLayerViewControllers.Last();
              this._NavStackLayerViewControllers.RemoveAt(this._NavStackLayerViewControllers.Count - 1);
              // Prep Destination View
              destinationVC = this._NavStackLayerViewControllers.Last();              
              showBackButton = (_NavStackLayerViewControllers.Count > 1);
              success = true;
            }
          }
          break;
            
        case OperationType.PopToRoot:
          {
            if (this._NavStackLayerViewControllers.Count > 1) {
              // Remove All Views except root
              sourceVC = this._NavStackLayerViewControllers.Last();
              destinationVC = this._NavStackLayerViewControllers.First();
              this._PopToViewController(destinationVC);
              showBackButton = false;
              success = true;
            }
          }
          break;
            
        case OperationType.PopToViewController:
          {
            if (this._NavStackLayerViewControllers.Contains(viewController)) {
              // Remove All Views on top of destination View
              sourceVC = this._NavStackLayerViewControllers.Last();
              this._PopToViewController(viewController);
              showBackButton = (_NavStackLayerViewControllers.Count > 1);
              success = true;
            }
          }
          break;
        }

        // Prep view for transition and perfrom Callbacks
        if (success) {

          destinationVC.NavigationController = this;
          destinationVC.gameObject.SetActive(true);
          // If DestinationVC is a NavigationController prepare Navigation Bar
          _PrepareDestinationVCNavigationController(destinationVC);
          // If SourceVC is a NavigationController prepare Navigation Bar
          _PrepareSourceVCNavigationController(sourceVC);
          // Setup Navigation Bar
          _UpdateNavBar(destinationVC, showBackButton);
          destinationVC.WillTransitionIn();
          sourceVC.WillTransitionOut();
        }

        // Set "Out" variables
        sourceViewController = sourceVC;
        destinationViewController = destinationVC;

        return success;
      }


      private void _PerformSegue(NavigationSegue segue) {
        if (segue.AnimationTransition == null) {
          DAS.Error("NavigationController", "_PerformSegueNavigationBeginAnimation Segue: " + segue.Identifier
          + " Does NOT have an Animation Transition");
          return;
        }

        // Block User Interaction
        CanvasGroup canvasGroup = GetComponent<CanvasGroup>();
        canvasGroup.interactable = false;

        // Perform transition Delegate
        if (segue.TransitionDelegate != null) {
          segue.TransitionDelegate(this);
        }

        // Call Action
        if (OnNavigationTransition != null) {
          OnNavigationTransition(segue.DestinationViewController, segue.AnimationTransition.AnimationDuration);
        }

        if (OnNavigationSegue != null) {
          OnNavigationSegue(segue);
        }
      
        segue.AnimationTransition.AnimationCompletedCallback = (() => {
          this._CompletedSegueTransition(segue);
        });

        segue.AnimationTransition.PerformAnimation();
      }

      
      // Perform cleanup and callbacks after segue has completed
      private void _CompletedSegueTransition(NavigationSegue segue) {
        // Perfrom Callbacks
        segue.DestinationViewController.DidTransitionIn();
        segue.SourceViewController.DidTransitionOut();
        segue.SourceViewController.gameObject.SetActive(false);
        
        // Post Operation Clean up and NavigationController Callbacks
        bool destroySource = true;
        switch (segue.OperationType) {
          
        case OperationType.Push:
          {
            // Need to reset sourceVC frame after animation
            this._PositionFullScreenView(segue.SourceViewController.gameObject);
            CanvasGroup sourceCG = segue.SourceViewController.gameObject.GetComponent<CanvasGroup>() as CanvasGroup;
            if (sourceCG != null) {
              sourceCG.alpha = 1.0f;
            }
            
            NavigationControllerDidPushViewController(segue.DestinationViewController);
            destroySource = false;
          }
          break;
            
        case OperationType.Replace:
          {
            NavigationControllerDidReplaceViewController(segue.SourceViewController, segue.DestinationViewController);            
          }
          break;
            
        case OperationType.Pop:
          {
            NavigationControllerDidPopViewController(segue.DestinationViewController);
          }
          break;
            
        case OperationType.PopToRoot:
          {
            NavigationControllerDidPopToRootViewController(segue.DestinationViewController);
          }
          break;
            
        case OperationType.PopToViewController:
          {
            NavigationControllerDidPopToViewController(segue.DestinationViewController);
          }
          break;
            
        case OperationType.Segue:
          {
            NavigationControllerDidPerfromSegue(segue.Identifier, segue.SourceViewController, segue.DestinationViewController);      
          }
          break;
        }

        if (destroySource) {
          segue.SourceViewController.DismissAllChildModals();
          Destroy(segue.SourceViewController.gameObject);
          Anki.AppResources.SpriteCache.RemoveUnusedSprites();
        }
        
        // Clean up Sugue
        Destroy(segue.AnimationTransition);

        // Un-Block User Interaction
        CanvasGroup canvasGroup = GetComponent<CanvasGroup>();
        canvasGroup.interactable = true;
      }

      
      private NavigationSegue _GenerateSegueForOperation(OperationType operationType,
                                                         ViewController sourceVC,
                                                         ViewController destinationVC,
                                                         SegueAnimationTransition.AnimationType animationType,
                                                         float animationDuration,
                                                         string customSegueIdentifier) {
        string segueIdentifier = null;
        NavigationSegueDirection direction = NavigationSegueDirection.Forward;

        switch (operationType) {
          
        case OperationType.Push:
          {
            segueIdentifier = PushSegueIdentifier;
            direction = NavigationSegueDirection.Forward;
          }
          break;
            
        case OperationType.Replace:
          {
            segueIdentifier = ReplaceSegueIdentifier;
            direction = NavigationSegueDirection.Forward;
          }
          break;
            
        case OperationType.Pop:
          {
            segueIdentifier = PopSegueIdentifier;
            direction = NavigationSegueDirection.Reverse;
          }
          break;
            
        case OperationType.PopToRoot:
          {
            segueIdentifier = PopToRootSegueIdentifier;
            direction = NavigationSegueDirection.Reverse;
          }
          break;
            
        case OperationType.PopToViewController:
          {
            segueIdentifier = PopToViewControllerSegueIdentifier;
            direction = NavigationSegueDirection.Reverse;
          }
          break;
            
        }

        // Apply custom segue identifier
        if (customSegueIdentifier != null) {
          segueIdentifier = customSegueIdentifier;
        }

        // Create Segue
        NavigationSegue segue = new NavigationSegue(sourceVC,
                                  destinationVC,
                                  segueIdentifier,
                                  direction,
                                  operationType);
        // Apply Animation
        if (segue != null) {
          SegueAnimationTransition.ApplyAnimationTransitionToSegue(this.gameObject,
            animationType,
            animationDuration,
            segue);
        }
        
        return segue;
      }


      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Private Nav Bar Setup

      // Setup Navigation Bar with Navigation Controller
      private void _SetUpNavBar() {
        if (NavigationBar != null) {
          // Add to Foreground view
          NavigationBar.gameObject.transform.SetParent(_ForegroundLayerView.transform, false);
          _LayoutNavBarTransform(NavigationBar.gameObject);
        }
      }
      
      // Update Navigation Bar for navigations operation
      private void _UpdateNavBar(ViewController viewController, bool showBackButton) {
        if (NavigationBar != null) {
          NavigationBar.Title = viewController.Title;
          NavigationBar.BackButtonHidden = !showBackButton;

          // Setup up actions
          NavigationBar.BackButton.onClick.RemoveAllListeners();
          NavigationBar.BackButton.onClick.AddListener(() => {
            
            ViewController visibleVC = VisibleViewController();
            visibleVC.NavigationControllerBackPressed(this);
          });
        }
      }

      // Setup Navigation Bar Layout
      private void _LayoutNavBarTransform(GameObject navigationBarGameObject) { 
        RectTransform rectTransform = navigationBarGameObject.GetComponent<RectTransform>();
        rectTransform.offsetMin = Vector2.zero;
        rectTransform.offsetMax = new Vector2(0.0f, _NavigationBarHeight);
        rectTransform.anchoredPosition = Vector2.zero;
        rectTransform.localScale = Vector3.one;
      }

    }
  }
}
