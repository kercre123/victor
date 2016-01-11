using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

namespace PatternPlay {

  public class PatternCollectionBankCard : MonoBehaviour {

    [SerializeField]
    private RectTransform _VerticalLayoutForRowPatterns;

    [SerializeField]
    private RectTransform _HorizontalLayoutForStackPatterns;

    [SerializeField]
    private RectTransform _HalfHorizontalLayoutForStackPatterns;

    [SerializeField]
    private PatternDisplay _RowPatternDisplayPrefab;

    [SerializeField]
    private PatternDisplay _StackPatternDisplayPrefab;

    [SerializeField]
    private Text _HeaderText;

    [SerializeField]
    private BadgeDisplay _NewBadgeDisplay;

    [SerializeField]
    private LayoutElement _LayoutElement;

    List<PatternDisplay> _PatternsShown;

    public void Initialize(MemoryBank memoryBank, float cardWidth) {
      _PatternsShown = new List<PatternDisplay>();

      // Depending on the signature, we need to use a particular layout
      switch (memoryBank.BankOrientation) {
      case MemoryBank.PatternOrientation.Horizontal:
        ShowRowPatterns(memoryBank);
        break;
      case MemoryBank.PatternOrientation.Vertical:
        ShowStackPatterns(memoryBank);
        break;
      case MemoryBank.PatternOrientation.Mixed:
        ShowSpecialPatterns(memoryBank);
        break;
      default:
        Debug.LogError("Not implemented!");
        break;
      }

      SetHeaderText(memoryBank);
      SetCardWidth(cardWidth);

      SetupBadge(memoryBank.Name);
    }

    public void RemoveBadgeIfSeen() {
      foreach (PatternDisplay display in _PatternsShown) {
        display.RemoveBadgeIfSeen();
      }
    }

    public bool HasAnyNewPatterns() {
      bool anyNewPatterns = false;
      foreach (PatternDisplay display in _PatternsShown) {
        if (display.Pattern != null && BadgeManager.DoesBadgeExistForKey(display.Pattern)) {
          anyNewPatterns = true;
          break;
        }
      }
      return anyNewPatterns;
    }

    public bool IsBadged() {
      return _NewBadgeDisplay.IsShowing();
    }

    private void SetCardWidth(float newWidth) {
      _LayoutElement.minWidth = newWidth;
    }

    private void SetHeaderText(MemoryBank memoryBank) {
      // TODO: Localize this?
      _HeaderText.text = memoryBank.Name;
    }

    private void SetupBadge(string tagName) {
      _NewBadgeDisplay.UpdateDisplayWithTag(tagName);
    }

    private void ShowRowPatterns(MemoryBank memoryBank) {
      ShowPatterns(memoryBank, _VerticalLayoutForRowPatterns, _RowPatternDisplayPrefab);
    }

    private void ShowStackPatterns(MemoryBank memoryBank) {
      ShowPatterns(memoryBank, _HorizontalLayoutForStackPatterns, _StackPatternDisplayPrefab);
    }

    private void ShowPatterns(MemoryBank memoryBank, RectTransform layoutContainer, PatternDisplay patternCardPrefab) {
      // Foreach seen pattern create a card
      HashSet<BlockPattern> patterns = memoryBank.Patterns;
      HashSet<BlockPattern> seenPatterns = memoryBank.GetSeenPatterns();
      if (patterns != null) {
        foreach (BlockPattern pattern in patterns) {
          if (seenPatterns != null && seenPatterns.Contains(pattern)) {
            CreatePatternCard(pattern, layoutContainer, patternCardPrefab);
          }
          else {
            CreatePatternCard(null, layoutContainer, patternCardPrefab);
          }
        }
      }
    }

    private void CreatePatternCard(BlockPattern pattern, RectTransform layoutContainer, PatternDisplay patternCardPrefab) {
      // Create the card
      GameObject newCard = Instantiate(patternCardPrefab.gameObject) as GameObject;

      // Add the card to the layout
      newCard.transform.SetParent(layoutContainer, false);

      // Update the card's display using the pattern
      PatternDisplay patternDisplay = newCard.GetComponent<PatternDisplay>();
      patternDisplay.Pattern = pattern;

      if (pattern != null) {
        _PatternsShown.Add(patternDisplay);
      }
    }

    private void ShowSpecialPatterns(MemoryBank memoryBank) {
      // Foreach seen pattern create a card
      HashSet<BlockPattern> patterns = memoryBank.Patterns;
      HashSet<BlockPattern> seenPatterns = memoryBank.GetSeenPatterns();
      RectTransform layoutContainer;
      PatternDisplay patternCardPrefab;
      if (patterns != null) {
        foreach (BlockPattern pattern in patterns) {
          if (pattern.VerticalStack) {
            layoutContainer = _HalfHorizontalLayoutForStackPatterns;
            patternCardPrefab = _StackPatternDisplayPrefab;
          }
          else {
            layoutContainer = _VerticalLayoutForRowPatterns;
            patternCardPrefab = _RowPatternDisplayPrefab;
          }
          if (seenPatterns != null && seenPatterns.Contains(pattern)) {
            CreatePatternCard(pattern, layoutContainer, patternCardPrefab);
          }
          else {
            CreatePatternCard(null, layoutContainer, patternCardPrefab);
          }
        }
      }
    }
  }

}
