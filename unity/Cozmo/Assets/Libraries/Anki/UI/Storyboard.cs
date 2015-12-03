using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Anki
{
  namespace UI
  {
    [RequireComponent(typeof(NavigationController))]

    public class Storyboard : Anki.BaseBehaviour
    {

      private StoryboardNode? _InitialNode;
      private StoryboardNode? _AlternativeInitialNode;
      private bool _DidLoadInitialNode = false;
      // Key = StoryboardNode Unique Identifiers
      private Dictionary<string, StoryboardNode> _StoryboardNodes = new Dictionary<string, StoryboardNode>();


      protected void SetInitialNode(StoryboardNode node)
      {
        _InitialNode = node;
        AddStoryboardNode(node);
      }


      /// <summary>
      /// Force load the inital storyboard node
      /// </summary>
      public void LoadInitialNode()
      {
        DAS.Debug("Storyboard", "_LoadInitialNode");
        if (!_DidLoadInitialNode) {
          _DidLoadInitialNode = true;
          // Load initial node
          StoryboardNode? initNode = null;
          if ( _AlternativeInitialNode.HasValue ) {
            initNode = _AlternativeInitialNode;
            _AlternativeInitialNode = null;
          }
          else if ( _InitialNode.HasValue ) {
            initNode = _InitialNode;
          }
          
          if ( initNode.HasValue ) {
            DAS.Debug("Storyboard", "_LoadInitialNode - Load Initial Node");
            // Load Initial ViewController
            ViewController vc = InstantiateViewControllerWithIdentifier(initNode.Value.Identifier);
            
            // Push Node
            if ( vc != null ) {
              _NavigationController.PushViewController(vc);
              _ApplyNodeNavigatoinControllerProperties(initNode.Value, _NavigationController);
            }
          }
        }
      }
      

      protected void AddStoryboardNode(StoryboardNode node)
      {
        if (!_StoryboardNodes.ContainsKey(node.Identifier)) {
          _StoryboardNodes.Add(node.Identifier, node);
        }
      }


      protected NavigationController _NavigationController {
        get { return GetComponent<NavigationController>(); }
      }


      public Anki.UI.ViewController InstantiateViewControllerWithIdentifier(string identifier)
      {
        StoryboardNode node;
        ViewController vc = null;
        if ( _StoryboardNodes.TryGetValue(identifier, out node) ) {
          // Init Node VC
          GameObject vcGameObj = PrefabLoader.Instance.InstantiatePrefab(node.ViewControllerPrefabName);
          if (vcGameObj == null) {
            DAS.Error("Storyboard", "InstantiateViewControllerWithIdentifier: " + identifier + " ViewControllerPrefab: " +
                      node.ViewControllerPrefabName + " does NOT exist");
          }
          vc = vcGameObj.GetComponent<ViewController>() as ViewController;
          vc.Storyboard = this;
          vc.StoryboardIdentifier = node.Identifier;
        }
        else {
          DAS.Error("Storyboard", "InstantiateViewControllerWithIdentifier: " + identifier + " does NOT exist");
        }
        return vc;
      }


      public void PerformSegue(Anki.UI.ViewController sourceViewController, string segueIdentifier)
      {
        // Check if ViewController & Segue with Identifier exist
        StoryboardNode node;
        if ( !_StoryboardNodes.TryGetValue(sourceViewController.StoryboardIdentifier, out node) ) {
          DAS.Error("Storyboard", "PerformSeugue - viewControllerIdentifier: " + sourceViewController.StoryboardIdentifier + " does NOT exist");
          return;
        }

        // Retrieve Segue Destination
        StoryboardSegueDescription segueDescription = node.SegueWithIdentifier(segueIdentifier);
        if ( segueDescription == null ) {
          DAS.Error("Storyboard", "PerformSeugue - segueIdentifier: " + segueIdentifier + " does NOT exist");
          return;
        }
        
        // Check Segue Type
        if (segueDescription is StoryboardExitSegueDescription) {
          // Perform segue on Parent Stroyboard
          NavigationController navCntr =_NavigationController;
          if (navCntr != null) {
            _NavigationController.PerformSegueWithIdentifier(((StoryboardExitSegueDescription)segueDescription).ExitSegueIdentifier);
          }
          else {
            DAS.Error("Storyboard", "PerformSeugue - segueIdentifier: " + segueIdentifier +" _NavigationController does NOT exist");
          }
        }
        else {
          // Create Destination View Controller
          // Retrieve Destination Storyboard Node
          StoryboardNode destinationNode;
          if ( !_StoryboardNodes.TryGetValue(segueDescription.DestinationStoryboardNodeIdentifier, out destinationNode) ) {
            DAS.Error("Storyboard", "PerformSeugue - DestinationStoryboardNodeIdentifier: " + segueDescription.DestinationStoryboardNodeIdentifier + " does NOT exist");
            return;
          }

          // Create Destination Node View Controller
          ViewController destinationVC = InstantiateViewControllerWithIdentifier(destinationNode.Identifier);
          if ( destinationVC == null ) {
            DAS.Error("Storyboard", "PerformSeugue - DestinationStoryboardNode DestinationVC: " + destinationNode.Identifier + " ViewControllerKey: " + destinationNode.ViewControllerPrefabName + " does NOT exist");
            return;
          }

          // Special Case - Destination is a Storyboard
          if (segueDescription is StoryboardToStoryboardSegueDescription) {
            Storyboard subSb = destinationVC.GetComponent<Storyboard>();
            if (subSb != null) {
              subSb.SetAlternativeInitalNode(((StoryboardToStoryboardSegueDescription)segueDescription).SubStoryboardInitialNodeIdentifier);
            }
            else {
              DAS.Error("Storyboard", "PerformSeugue - DestinationViewController: " + destinationNode.Identifier + " Does NOT have a Storyboard component");
            }
          }

          // Create Segue
          NavigationSegue segue = new NavigationSegue(sourceViewController, destinationVC,
                                                      segueDescription.Identifier,
                                                      segueDescription.Direction);
          // Create Animation
          SegueAnimationTransition.ApplyAnimationTransitionToSegue(this.gameObject, 
                                                                   segueDescription.AnimationType,
                                                                   segueDescription.AnimationDuration,
                                                                   segue);

          // Animation Transition Block
          segue.TransitionDelegate = (NavigationController navigationController) => {
            _ApplyNodeNavigatoinControllerProperties(destinationNode, navigationController);
          };

          // Perform segue
          if ( sourceViewController.NavigationController == null ) {
            DAS.Error("Storyboard", "PerformSeugue - SourceViewController: " + sourceViewController.StoryboardIdentifier + " NavigationController is NULL");
            return;
          }

          sourceViewController.NavigationController.PerformSegueNavigation(segue);
        }
      }


      public void PerformDefaultUnwindSegue(Anki.UI.ViewController sourceViewController)
      {
        // Search for unwind segue
        // Check if ViewController & Segue with Identifier exist
        StoryboardNode node;
        if ( !_StoryboardNodes.TryGetValue(sourceViewController.StoryboardIdentifier, out node) ) {
          DAS.Error("Storyboard", "PerformDefaultUnwindSegue - viewControllerIdentifier: " + sourceViewController.StoryboardIdentifier + " does NOT exist");
          return;
        }

        // Enumerate through segues to find a single unwind segue
        StoryboardSegueDescription segueDescription = node.DefaultUnwindSegue();
        if ( segueDescription != null ) {
          PerformSegue(sourceViewController, segueDescription.Identifier);
        }
        else {
          DAS.Error("Storyboard", "PerformDefaultUnwindSegue - viewControllerIdentifier: " + sourceViewController.StoryboardIdentifier + " does NOT have a default Unwind Segue");
        }
      }


      protected override void OnLoad()
      {   
        base.OnLoad();
        
        LoadStoryboard();
      }


      protected void Start()
      {
        LoadInitialNode();
      }


      // TODO: Need to figure out the best way to load meta data
      // Currently subclassing Storyborad for specific navigation
      protected virtual void LoadStoryboard()
      {
      }


      protected void _ApplyNodeNavigatoinControllerProperties(StoryboardNode node, NavigationController navigationController)
      {
        if (navigationController != null) {
          navigationController.BackgroundViewHidden = node.NavigationBackgroundViewHidden;
          navigationController.ForegroundViewHidden = node.NavigationForegroundViewHidden;
          navigationController.NavigationBarHidden = node.NavigationBarHidden;
          if (navigationController.NavigationBar != null) {
            navigationController.NavigationBar.BackButtonHidden = node.NavigationBarBackButtonHidden;
          }
        }
      }


      public void SetAlternativeInitalNode(string nodeIdentifier)
      {
        StoryboardNode node;
        if (_StoryboardNodes.TryGetValue(nodeIdentifier, out node)) {
          _AlternativeInitialNode = node;
          // Reset Inital Load
          _DidLoadInitialNode = false;
        }
        else {
          DAS.Error("Storyboard", "SetAlternativeInitalNode - nodeIdentifier: " + nodeIdentifier + " Node does NOT exist");
        }
      }
    }

  }
}
