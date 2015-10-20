using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class PatternCollectionDialog : MonoBehaviour {

  [SerializeField]
  private PatternCollectionBankCard _memoryBankCardPrefab;

  [SerializeField]
  private RectTransform _memoryBankCardContentPanel;

  [SerializeField]
  private Text _percentCompleteLabel;

  private Dictionary<MemoryBank, PatternCollectionBankCard> _signatureToCardDict;

	public void Initialize(PatternMemory patternMemory){
    // Create all the memory bank cards
    _signatureToCardDict = CreateMemoryBankCards (patternMemory);

    // TODO: Listen for added patterns and update dialog - shouldn't update every frame
    // We'll need to cache the cards for easy access later

    SetCompletionText (patternMemory.GetNumSeenPatterns (), patternMemory.GetNumTotalPatterns ());
  }
  
  private Dictionary<MemoryBank, PatternCollectionBankCard> CreateMemoryBankCards(PatternMemory patternMemory) {
    Dictionary<MemoryBank, PatternCollectionBankCard> memoryBankCards 
    = new Dictionary<MemoryBank, PatternCollectionBankCard>();

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

        // Add the card to the dictionary
        memoryBankCards.Add ( bank, bankCardScript );
      }
    }

    return memoryBankCards;
  }

  public void OnCloseButtonTap()
  {
    UIManager.CloseDialog (this.gameObject);
  }

  private void SetCompletionText(int numSeenPatterns, int numTotalPatterns)
  {
    float percentComplete = numSeenPatterns / (float)numTotalPatterns;
    percentComplete *= 100;
    _percentCompleteLabel.text = string.Format ("{0:N1}% Complete", percentComplete);
  }
}
