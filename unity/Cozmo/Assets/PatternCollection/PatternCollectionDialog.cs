using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class PatternCollectionDialog : BaseDialog {

  [SerializeField]
  private PatternCollectionBankCard _memoryBankCardPrefab;

  [SerializeField]
  private RectTransform _memoryBankCardContentPanel;

  [SerializeField]
  private Text _percentCompleteLabel;

  [SerializeField]
  private ScrollRect _memoryBankScrollRect;

  private List<PatternCollectionBankCard> _memoryBankCards;

  [SerializeField]
  private float _slowDragSpeedThreshold = 30f;

	public void Initialize(PatternMemory patternMemory){
    // Create all the memory bank cards
    _memoryBankCards = CreateMemoryBankCards (patternMemory);

    // TODO: Listen for added patterns and update dialog - shouldn't update every frame
    // We'll need to cache the cards for easy access later

    SetCompletionText (patternMemory.GetNumSeenPatterns (), patternMemory.GetNumTotalPatterns ());
  }

  public void OnDrag()
  {
    // The player could be dragging the scrollview slowly while 
    // looking at the cards at the same time. We want to remove the badge
    // if the player has seen it.
    float scrollXSpeed = Mathf.Abs(_memoryBankScrollRect.velocity.x);
    if (scrollXSpeed < _slowDragSpeedThreshold) {
      RemoveBadgesIfSeen();
    }
  }

  public void OnDragEnd()
  {
    // We want to remove the badge if the player has seen it, 
    // which is generally when they stop scrolling.
    RemoveBadgesIfSeen ();
  }

  private void RemoveBadgesIfSeen()
  {
    foreach (PatternCollectionBankCard card in _memoryBankCards) {
      card.RemoveBadgeIfSeen();
    }
  }
  
  private List<PatternCollectionBankCard> CreateMemoryBankCards(PatternMemory patternMemory) {
    List<PatternCollectionBankCard> memoryBankCards = new List<PatternCollectionBankCard>();

    // Foreach memory bank...
    List<MemoryBank> memoryBanks = patternMemory.GetMemoryBanks ();
    GameObject newBankCard;
    PatternCollectionBankCard bankCardScript;
    foreach (MemoryBank bank in memoryBanks) {
      // Only show cards if the player has seen at least one pattern
      if (bank.GetSeenPatterns().Count > 0)
      {
        // Create a card
        newBankCard = Instantiate(_memoryBankCardPrefab.gameObject) as GameObject;

        // Layout the card into the scrollable panel
        // The layout will modify the position, so make sure that FALSE is 
        // there so that the world position will update.
        newBankCard.transform.SetParent(_memoryBankCardContentPanel, false);

        // Initialize the card with the bank
        bankCardScript = newBankCard.GetComponent<PatternCollectionBankCard>();
        bankCardScript.Initialize(bank);

        // Add the card to the list
        memoryBankCards.Add ( bankCardScript );
      }
    }

    return memoryBankCards;
  }

  public void OnCloseButtonTap()
  {
    RemoveBadgesIfSeen ();
    UIManager.CloseDialog (this);
  }

  private void SetCompletionText(int numSeenPatterns, int numTotalPatterns)
  {
    float percentComplete = numSeenPatterns / (float)numTotalPatterns;
    percentComplete *= 100;
    _percentCompleteLabel.text = string.Format ("{0:N1}% Complete", percentComplete);
  }

  public void SetScrollValue(float scrollValue)
  {
    float clampValue = Mathf.Clamp (scrollValue, 0, 1);
    _memoryBankScrollRect.horizontalNormalizedPosition = clampValue;
  }

  public float GetScrollValue()
  {
    return _memoryBankScrollRect.horizontalNormalizedPosition;
  }
}
