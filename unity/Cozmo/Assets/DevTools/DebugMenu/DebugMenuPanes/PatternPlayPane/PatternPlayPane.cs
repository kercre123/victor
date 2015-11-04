using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class PatternPlayPane : MonoBehaviour {

  public delegate void PatternPlayPaneOpenHandler(PatternPlayPane patternPlayPane);
  public static event PatternPlayPaneOpenHandler PatternPlayPaneOpened;
  private static void RaisePatternPlayPaneOpened(PatternPlayPane patternPlayPane){
    if (PatternPlayPaneOpened != null) {
      PatternPlayPaneOpened(patternPlayPane);
    }
  }

  private PatternCollectionViewController _patternPlayViewInstance = null;

  [SerializeField]
  private Button _createTestMemoryButton;

  [SerializeField]
  private Button _addRandomPatternButton;

  [SerializeField]
  private Button _addAllPatternsButton;

  private void Start(){
    RaisePatternPlayPaneOpened(this);
  }

  private void OnDestroy(){
    _createTestMemoryButton.onClick.RemoveListener(OnCreateTestMemoryButtonTap);
    _addRandomPatternButton.onClick.RemoveListener(OnAddRandomPatternButtonTap);
    _addAllPatternsButton.onClick.RemoveListener(OnAddAllPatternsButtonTap);
  }

  public void Initialize(PatternCollectionViewController viewController){
    _patternPlayViewInstance = viewController;

    if (_patternPlayViewInstance.IsInitialized()) {
      _createTestMemoryButton.interactable = false;
      _addRandomPatternButton.interactable = true;
      _addAllPatternsButton.interactable = true;
    }
    else {
      _createTestMemoryButton.interactable = true;
      _addRandomPatternButton.interactable = false;
      _addAllPatternsButton.interactable = false;
      _createTestMemoryButton.onClick.AddListener(OnCreateTestMemoryButtonTap);
    }

    _addRandomPatternButton.onClick.AddListener(OnAddRandomPatternButtonTap);
    _addAllPatternsButton.onClick.AddListener(OnAddAllPatternsButtonTap);
  }

  private void OnCreateTestMemoryButtonTap(){
    CreateTestPatternMemory();
    _createTestMemoryButton.interactable = false;
    _addRandomPatternButton.interactable = true;
    _addAllPatternsButton.interactable = true;
  }
  
  private void OnAddRandomPatternButtonTap(){
    bool success = AddRandomPattern();
    _addRandomPatternButton.interactable = success;
    _addAllPatternsButton.interactable = success;
  }
  
  private void OnAddAllPatternsButtonTap(){
    AddAllPatterns();
    _addAllPatternsButton.interactable = false;
    _addRandomPatternButton.interactable = false;
  }
  
  #region UI Testing Helpers
  public void CreateTestPatternMemory() {
    if (_patternPlayViewInstance.IsInitialized()) {
      return;
    }

    PatternMemory patternMemory = new PatternMemory ();
    patternMemory.Initialize ();
    
    // Add fake seen patterns to memory
    BlockPattern newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = true },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = true },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = true }
    };
    newPattern.verticalStack = true;
    patternMemory.AddSeen (newPattern);
    
    BadgeManager.TryRemoveBadge (newPattern);
    
    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = false, left = false, right = true, facing_cozmo = true },
      new BlockLights{ front = false, back = false, left = false, right = true, facing_cozmo = true },
      new BlockLights{ front = false, back = false, left = false, right = true, facing_cozmo = true }
    };
    newPattern.verticalStack = true;
    patternMemory.AddSeen (newPattern);
    
    BadgeManager.TryRemoveBadge (newPattern);
    
    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = true, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = true, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = true, right = false, facing_cozmo = false }
    };
    newPattern.verticalStack = false;
    patternMemory.AddSeen (newPattern);
    BadgeManager.TryRemoveBadge (newPattern);
    
    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = true, back = true, left = true, right = false, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = false, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = false, facing_cozmo = true }
    };
    newPattern.verticalStack = false;
    patternMemory.AddSeen (newPattern);
    BadgeManager.TryRemoveBadge (newPattern);
    
    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = false }
    };
    newPattern.verticalStack = true;
    patternMemory.AddSeen (newPattern);
    BadgeManager.TryRemoveBadge (newPattern);
    
    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = true }
    };
    newPattern.verticalStack = true;
    patternMemory.AddSeen (newPattern);
    
    newPattern = new BlockPattern ();
    newPattern.blocks = new List<BlockLights> {
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = false },
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = false },
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = false }
    };
    newPattern.verticalStack = false;
    patternMemory.AddSeen (newPattern);
    
    _patternPlayViewInstance.Initialize (patternMemory);
  }

  public bool AddRandomPattern() {
    bool addedPattern = false;
    if (_patternPlayViewInstance.AnyUnseenPatterns()) {
      do {
        bool randFront = FlipCoin();
        bool randBack = FlipCoin();
        bool randLeft = FlipCoin();
        bool randRight = FlipCoin();
        bool randFacing = FlipCoin();
        bool randStack = FlipCoin();
      
        BlockPattern newPattern = new BlockPattern();
        newPattern.blocks = new List<BlockLights> {
        new BlockLights{ front = randFront, back = randBack, left = randLeft, right = randRight, facing_cozmo = randFacing },
        new BlockLights{ front = randFront, back = randBack, left = randLeft, right = randRight, facing_cozmo = randFacing },
        new BlockLights{ front = randFront, back = randBack, left = randLeft, right = randRight, facing_cozmo = randFacing }
      };
        newPattern.verticalStack = randStack;
        addedPattern = _patternPlayViewInstance.TryAddPattern(newPattern);
      } while (!addedPattern);
    }
    return addedPattern;
  }
  
  private bool FlipCoin()
  {
    return Random.Range (0f, 1f) > 0.5f;
  }
  
  public void AddAllPatterns() {
    _patternPlayViewInstance.AddAllUnseenPatterns ();
  }
  #endregion
}
