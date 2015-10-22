using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class PatternCollectionBankCard : MonoBehaviour {

  [SerializeField]
  private RectTransform _verticalLayoutForRowPatterns;

  [SerializeField]
  private RectTransform _horizontalLayoutForStackPatterns;

  [SerializeField]
  private RectTransform _halfHorizontalLayoutForStackPatterns;

  [SerializeField]
  private PatternDisplay _rowPatternDisplayPrefab;

  [SerializeField]
  private PatternDisplay _stackPatternDisplayPrefab;

  [SerializeField]
  private Text _headerText;
  
  [SerializeField]
  private BadgeDisplay _newBadgeDisplay;
  
  [SerializeField]
  private LayoutElement _layoutElement;

  List<PatternDisplay> _patternsShown;

  public void Initialize(MemoryBank memoryBank, float cardWidth) {
    _patternsShown = new List<PatternDisplay> ();

    // Depending on the signature, we need to use a particular layout
    switch (memoryBank.BankOrientation) {
    case MemoryBank.PatternOrientation.Horizontal:
      ShowRowPatterns (memoryBank);
      break;
    case MemoryBank.PatternOrientation.Vertical:
      ShowStackPatterns (memoryBank);
      break;
    case MemoryBank.PatternOrientation.Mixed:
      ShowSpecialPatterns (memoryBank);
      break;
    default:
      Debug.LogError ("Not implemented!");
      break;
    }

    SetHeaderText (memoryBank);
    SetCardWidth (cardWidth);

    SetupBadge (memoryBank.name);
  }

  public void RemoveBadgeIfSeen()
  {
    foreach (PatternDisplay display in _patternsShown) {
      display.RemoveBadgeIfSeen();
    }
  }

  public bool HasAnyNewPatterns()
  {
    bool anyNewPatterns = false;
    foreach (PatternDisplay display in _patternsShown) {
      if (display.pattern != null && BadgeManager.DoesBadgeExistForKey(display.pattern)){
        anyNewPatterns = true;
        break;
      }
    }
    return anyNewPatterns;
  }

  public bool IsBadged()
  {
    return _newBadgeDisplay.IsShowing ();
  }

  private void SetCardWidth(float newWidth) {
    _layoutElement.minWidth = newWidth;
  }

  private void SetHeaderText(MemoryBank memoryBank) {
    // TODO: Localize this?
    _headerText.text = memoryBank.name;
  }

  private void SetupBadge(string tagName)
  {
    _newBadgeDisplay.UpdateDisplayWithTag (tagName);
  }

  private void ShowRowPatterns(MemoryBank memoryBank)
  {
    ShowPatterns (memoryBank, _verticalLayoutForRowPatterns, _rowPatternDisplayPrefab);
  }

  private void ShowStackPatterns(MemoryBank memoryBank)
  {
    ShowPatterns (memoryBank, _horizontalLayoutForStackPatterns, _stackPatternDisplayPrefab);
  }

  private void ShowPatterns(MemoryBank memoryBank, RectTransform layoutContainer, PatternDisplay patternCardPrefab)
  {
    // Foreach seen pattern create a card
    HashSet<BlockPattern> patterns = memoryBank.patterns;
    HashSet<BlockPattern> seenPatterns = memoryBank.GetSeenPatterns ();
    if (patterns != null) {
      foreach (BlockPattern pattern in patterns) {
        if (seenPatterns != null && seenPatterns.Contains(pattern))
        {
          CreatePatternCard(pattern, layoutContainer, patternCardPrefab);
        }
        else
        {
          CreatePatternCard(null, layoutContainer, patternCardPrefab);
        }
      }
    }
  }

  private void CreatePatternCard(BlockPattern pattern, RectTransform layoutContainer, PatternDisplay patternCardPrefab)
  {
    // Create the card
    GameObject newCard = Instantiate (patternCardPrefab.gameObject) as GameObject;

    // Add the card to the layout
    newCard.transform.SetParent (layoutContainer, false);

    // Update the card's display using the pattern
    PatternDisplay patternDisplay = newCard.GetComponent<PatternDisplay> ();
    patternDisplay.pattern = pattern;

    if (pattern != null) {
      _patternsShown.Add (patternDisplay);
    }
  }

  private void ShowSpecialPatterns(MemoryBank memoryBank)
  {
    // Foreach seen pattern create a card
    HashSet<BlockPattern> patterns = memoryBank.patterns;
    HashSet<BlockPattern> seenPatterns = memoryBank.GetSeenPatterns ();
    RectTransform layoutContainer;
    PatternDisplay patternCardPrefab;
    if (patterns != null) {
      foreach (BlockPattern pattern in patterns) {
        if (pattern.verticalStack) {
          layoutContainer = _halfHorizontalLayoutForStackPatterns;
          patternCardPrefab = _stackPatternDisplayPrefab;
        }
        else {
          layoutContainer = _verticalLayoutForRowPatterns;
          patternCardPrefab = _rowPatternDisplayPrefab;
        }
        if (seenPatterns != null && seenPatterns.Contains(pattern))
        {
          CreatePatternCard(pattern, layoutContainer, patternCardPrefab);
        }
        else
        {
          CreatePatternCard(null, layoutContainer, patternCardPrefab);
        }
      }
    }
  }
}
