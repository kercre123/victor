using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class PatternCollectionBankCard : MonoBehaviour {
  
  // TODO: Remove magic numbers! Assume 4 matches per memory bank
  private const int PATTERN_PER_BANK_COUNT = 4;

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

  public void Initialize(MemoryBank memoryBank) {
    // Depending on the signature, we need to use a particular layout
    switch (memoryBank.Signature.verticalStack) {
    case BankRequirementFlag.FALSE:
      ShowRowPatterns (memoryBank);
      break;
    case BankRequirementFlag.TRUE:
      ShowStackPatterns (memoryBank);
      break;
    case BankRequirementFlag.NOT_REQUIRED:
      ShowSpecialPatterns (memoryBank);
      break;
    default:
      Debug.LogError ("Not implemented!");
      break;
    }

    SetHeaderText ();
  }

  private void SetHeaderText() {
    // TODO: Localize this?
    _headerText.text = "Memory Bank";
  }

  private void ShowRowPatterns(MemoryBank memoryBank)
  {
    ShowPatterns (memoryBank.GetSeenPatterns (), _verticalLayoutForRowPatterns, _rowPatternDisplayPrefab);
  }

  private void ShowStackPatterns(MemoryBank memoryBank)
  {
    ShowPatterns (memoryBank.GetSeenPatterns (), _horizontalLayoutForStackPatterns, _stackPatternDisplayPrefab);
  }

  private void ShowPatterns(IEnumerable<BlockPattern> patterns, RectTransform layoutContainer, PatternDisplay patternCardPrefab)
  {
    // Foreach seen pattern create a card
    int createdCards = 0;
    if (patterns != null) {
      foreach (BlockPattern pattern in patterns) {
        CreatePatternCard(pattern, layoutContainer, patternCardPrefab);
        createdCards++;
      }
    }

    // Fill out the rest of the layout with empty cards
    for (int i = 0; i < PATTERN_PER_BANK_COUNT - createdCards; i++) {
      CreatePatternCard(null, layoutContainer, patternCardPrefab);
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
  }

  private void ShowSpecialPatterns(MemoryBank memoryBank)
  {
    IEnumerable<BlockPattern> patterns = memoryBank.GetSeenPatterns ();
    int createdStacks = 0;
    int createdRows = 0;

    if (patterns != null) {
      foreach (BlockPattern pattern in patterns) {
        if (pattern.verticalStack) {
          CreatePatternCard(pattern, _halfHorizontalLayoutForStackPatterns, _stackPatternDisplayPrefab);
          createdStacks++;
        }
        else {
          CreatePatternCard(pattern, _verticalLayoutForRowPatterns, _rowPatternDisplayPrefab);
          createdRows++;
        }
      }
    }

    // Fill out the rest of the layout with empty cards
    for (int i = 0; i < 2 - createdRows; i++) {
      CreatePatternCard(null, _verticalLayoutForRowPatterns, _rowPatternDisplayPrefab);
    }
    for (int i = 0; i < 2 - createdStacks; i++) {
      CreatePatternCard(null, _halfHorizontalLayoutForStackPatterns, _stackPatternDisplayPrefab);
    }
  }
}
