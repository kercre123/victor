using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class PatternPlayPane : MonoBehaviour {

  public delegate void PatternPlayPaneOpenHandler(PatternPlayPane patternPlayPane);

  public static event PatternPlayPaneOpenHandler PatternPlayPaneOpened;

  private static void RaisePatternPlayPaneOpened(PatternPlayPane patternPlayPane) {
    if (PatternPlayPaneOpened != null) {
      PatternPlayPaneOpened(patternPlayPane);
    }
  }

  private PatternCollectionViewController _PatternPlayViewInstance = null;

  [SerializeField]
  private Button _CreateTestPatternMemory;

  [SerializeField]
  private Button _AddRandomPatternButton;

  [SerializeField]
  private Button _AddAllPatternsButton;

  private void Start() {
    RaisePatternPlayPaneOpened(this);
  }

  private void OnDestroy() {
    _CreateTestPatternMemory.onClick.RemoveListener(OnCreateTestMemoryButtonTap);
    _AddRandomPatternButton.onClick.RemoveListener(OnAddRandomPatternButtonTap);
    _AddAllPatternsButton.onClick.RemoveListener(OnAddAllPatternsButtonTap);
  }

  public void Initialize(PatternCollectionViewController viewController) {
    _PatternPlayViewInstance = viewController;

    if (_PatternPlayViewInstance.IsInitialized()) {
      _CreateTestPatternMemory.interactable = false;
      _AddRandomPatternButton.interactable = true;
      _AddAllPatternsButton.interactable = true;
    }
    else {
      _CreateTestPatternMemory.interactable = true;
      _AddRandomPatternButton.interactable = false;
      _AddAllPatternsButton.interactable = false;
      _CreateTestPatternMemory.onClick.AddListener(OnCreateTestMemoryButtonTap);
    }

    _AddRandomPatternButton.onClick.AddListener(OnAddRandomPatternButtonTap);
    _AddAllPatternsButton.onClick.AddListener(OnAddAllPatternsButtonTap);
  }

  private void OnCreateTestMemoryButtonTap() {
    CreateTestPatternMemory();
    _CreateTestPatternMemory.interactable = false;
    _AddRandomPatternButton.interactable = true;
    _AddAllPatternsButton.interactable = true;
  }

  private void OnAddRandomPatternButtonTap() {
    bool success = AddRandomPattern();
    _AddRandomPatternButton.interactable = success;
    _AddAllPatternsButton.interactable = success;
  }

  private void OnAddAllPatternsButtonTap() {
    AddAllPatterns();
    _AddAllPatternsButton.interactable = false;
    _AddRandomPatternButton.interactable = false;
  }

  #region UI Testing Helpers

  public void CreateTestPatternMemory() {
    if (_PatternPlayViewInstance.IsInitialized()) {
      return;
    }

    PatternMemory patternMemory = new PatternMemory();
    patternMemory.Initialize();
    
    // Add fake seen patterns to memory
    BlockPattern newPattern = new BlockPattern();
    newPattern.blocks_ = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = true },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = true },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = true }
    };
    newPattern.verticalStack_ = true;
    patternMemory.AddSeen(newPattern); 
    
    BadgeManager.TryRemoveBadge(newPattern);
    
    newPattern = new BlockPattern();
    newPattern.blocks_ = new List<BlockLights> {
      new BlockLights{ front = false, back = false, left = false, right = true, facing_cozmo = true },
      new BlockLights{ front = false, back = false, left = false, right = true, facing_cozmo = true },
      new BlockLights{ front = false, back = false, left = false, right = true, facing_cozmo = true }
    };
    newPattern.verticalStack_ = true;
    patternMemory.AddSeen(newPattern);
    
    BadgeManager.TryRemoveBadge(newPattern);
    
    newPattern = new BlockPattern();
    newPattern.blocks_ = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = true, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = true, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = true, right = false, facing_cozmo = false }
    };
    newPattern.verticalStack_ = false;
    patternMemory.AddSeen(newPattern);
    BadgeManager.TryRemoveBadge(newPattern);
    
    newPattern = new BlockPattern();
    newPattern.blocks_ = new List<BlockLights> {
      new BlockLights{ front = true, back = true, left = true, right = false, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = false, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = false, facing_cozmo = true }
    };
    newPattern.verticalStack_ = false;
    patternMemory.AddSeen(newPattern);
    BadgeManager.TryRemoveBadge(newPattern);
    
    newPattern = new BlockPattern();
    newPattern.blocks_ = new List<BlockLights> {
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = false },
      new BlockLights{ front = false, back = true, left = false, right = false, facing_cozmo = false }
    };
    newPattern.verticalStack_ = true;
    patternMemory.AddSeen(newPattern);
    BadgeManager.TryRemoveBadge(newPattern);
    
    newPattern = new BlockPattern();
    newPattern.blocks_ = new List<BlockLights> {
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = true },
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = true }
    };
    newPattern.verticalStack_ = true;
    patternMemory.AddSeen(newPattern);
    
    newPattern = new BlockPattern();
    newPattern.blocks_ = new List<BlockLights> {
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = false },
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = false },
      new BlockLights{ front = true, back = true, left = true, right = true, facing_cozmo = false }
    };
    newPattern.verticalStack_ = false;
    patternMemory.AddSeen(newPattern);
    
    _PatternPlayViewInstance.Initialize(patternMemory);
  }

  public bool AddRandomPattern() {
    bool addedPattern = false;
    if (_PatternPlayViewInstance.AnyUnseenPatterns()) {
      do {
        bool randFront = FlipCoin();
        bool randBack = FlipCoin();
        bool randLeft = FlipCoin();
        bool randRight = FlipCoin();
        bool randFacing = FlipCoin();
        bool randStack = FlipCoin();
      
        BlockPattern newPattern = new BlockPattern();
        newPattern.blocks_ = new List<BlockLights> {
          new BlockLights {
            front = randFront,
            back = randBack,
            left = randLeft,
            right = randRight,
            facing_cozmo = randFacing
          },
          new BlockLights {
            front = randFront,
            back = randBack,
            left = randLeft,
            right = randRight,
            facing_cozmo = randFacing
          },
          new BlockLights {
            front = randFront,
            back = randBack,
            left = randLeft,
            right = randRight,
            facing_cozmo = randFacing
          }
        };
        newPattern.verticalStack_ = randStack;
        addedPattern = _PatternPlayViewInstance.TryAddPattern(newPattern);
      } while (!addedPattern);
    }
    return addedPattern;
  }

  private bool FlipCoin() {
    return Random.Range(0f, 1f) > 0.5f;
  }

  public void AddAllPatterns() {
    _PatternPlayViewInstance.AddAllUnseenPatterns();
  }

  #endregion
}
