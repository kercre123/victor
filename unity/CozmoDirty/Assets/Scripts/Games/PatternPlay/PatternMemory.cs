using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class PatternMemory {

  public const string PATTERN_MEMORY_BADGE_TAG = "PatternMemory";

  public delegate void PatternHandler(BlockPattern patternAdded, MemoryBank bankParent);
  public event PatternHandler PatternAdded;
  public void RaisePatternAdded(BlockPattern pattern, MemoryBank bank){
    if (PatternAdded != null) {
      PatternAdded (pattern, bank);
    }
  }

  private List<MemoryBank> memoryBanks = new List<MemoryBank>();
  private HashSet<BlockPattern> keyPatterns = new HashSet<BlockPattern>();

  public void Initialize() {

    // Load the pattern from the text asset
    TextAsset patternMemoryAsset = Resources.Load("Data/BlockPatterns", typeof(TextAsset)) as TextAsset;
    memoryBanks = LoadPatterns(patternMemoryAsset);

    // adding key patterns.

  }

  // Pattern Attributes
  private static string kPatternAttribVertical = "vertical";
  private static string kPatternAttribBlocks = "blocks";

  //Block Attributes
  private static string kBlockAttribFront = "front";
  private static string kBlockAttribBack = "back";
  private static string kBlockAttribLeft = "left";
  private static string kBlockAttribRight = "right";
  private static string kBlockAttribFacingCozmo = "facing_cozmo";

  private List<MemoryBank> LoadPatterns(TextAsset asset) {
    List<MemoryBank> result = new List<MemoryBank>();
    Debug.Assert(asset != null);
    JSONObject patternMemoryObj = new JSONObject(asset.text);
    Debug.Log(asset);
    // Unpack the patterns
    Debug.Assert(patternMemoryObj.IsObject);
    foreach (string key in patternMemoryObj.keys) {
      MemoryBank bank = new MemoryBank(key);
      JSONObject bankObj = patternMemoryObj[key];
      Debug.Assert(bankObj.IsArray);
      foreach (JSONObject pattern in bankObj.list) {
        BlockPattern curPattern = new BlockPattern();
        curPattern.verticalStack = pattern[kPatternAttribVertical].b;
        curPattern.facingCozmo = pattern[kPatternAttribBlocks].b;
        Debug.Assert(pattern[kPatternAttribBlocks].IsArray);
        foreach (JSONObject blockData in pattern[kPatternAttribBlocks].list) {
          Debug.Assert(blockData.IsObject);
          BlockLights curBlock = new BlockLights();
          curBlock.front = (blockData[kBlockAttribFront].str != "");
          curBlock.back = (blockData[kBlockAttribBack].str != "");
          curBlock.left = (blockData[kBlockAttribLeft].str != "");
          curBlock.right = (blockData[kBlockAttribRight].str != "");
          curBlock.facing_cozmo = blockData[kBlockAttribFacingCozmo].b;
          curPattern.blocks.Add(curBlock);
        }
        bank.Add(curPattern);
        DAS.Info("PatternMemory.Init", "Pattern Added for bank: " + key + " hash = " + curPattern.GetHashCode());
      }
      result.Add(bank);
    }
    return result;
  }

  public List<MemoryBank> GetMemoryBanks() {
    return memoryBanks;
  }

  public bool AddSeen(BlockPattern pattern) {
    bool newPattern = false;
    DAS.Info("PatternMemory.AddSeen", "Seen pattern " + pattern.GetHashCode());
    for (int i = 0; i < memoryBanks.Count; ++i) {
      if (memoryBanks[i].Contains(pattern)) {
        if (memoryBanks[i].TryAddSeen(pattern)) {
          DAS.Info("PatternMemory.Add", "Pattern Added for bank: " + memoryBanks[i].name);

          newPattern = true;

          List<string> tags = new List<string>();
          tags.Add(PATTERN_MEMORY_BADGE_TAG);
          tags.Add(memoryBanks[i].name);
          BadgeManager.TryAddBadge(pattern, tags);
          RaisePatternAdded(pattern, memoryBanks[i]);
        }
        else {
          DAS.Info("PatternMemory.Add", "Pattern already exists in bank: " + memoryBanks[i].name);
          newPattern = false;
        }
        DAS.Info("PatternMemory.Add", "There are now " + memoryBanks[i].GetSeenPatterns().Count + " patterns in that bank");
        return newPattern;
      }
    }

    DAS.Info("PatternMemory.Add.InvalidPattern", "pattern does not match any registered in PatternMemory");
    return false;
  }

  public bool ContainsSeen(BlockPattern pattern) {
    for (int i = 0; i < memoryBanks.Count; ++i) {
      if (memoryBanks[i].GetSeenPatterns().Contains(pattern)) {
        return true;
      }
    }
    return false;
  }

  // Returns: null if no bank has a given pattern.
  public MemoryBank GetBankForPattern(BlockPattern pattern) {
    for (int i = 0; i < memoryBanks.Count; ++i) {
      if (memoryBanks[i].Contains(pattern)) {
        return memoryBanks[i];
      }
    }
    return null;
  }

  public HashSet<BlockPattern> GetAllPatterns() {
    HashSet<BlockPattern> allPatterns = new HashSet<BlockPattern>();
    for (int i = 0; i < memoryBanks.Count; i++) {
      allPatterns.UnionWith(memoryBanks[i].patterns);
    }
    return allPatterns;
  }

  public HashSet<BlockPattern> GetAllSeenPatterns() {
    HashSet<BlockPattern> seenPatterns = new HashSet<BlockPattern>();
    for (int i = 0; i < memoryBanks.Count; i++) {
      seenPatterns.UnionWith(memoryBanks[i].GetSeenPatterns());
    }
    return seenPatterns;
  }

  public HashSet<BlockPattern> GetAllUnseenPatterns() {
    HashSet<BlockPattern> seenPatterns = GetAllSeenPatterns();
    HashSet<BlockPattern> result = GetAllPatterns();
    result.ExceptWith(seenPatterns);
    return result;
  }

  public BlockPattern GetAnUnseenPattern() {
    HashSet<BlockPattern> allUnseenPatterns = GetAllUnseenPatterns();
    int i = Random.Range(0, allUnseenPatterns.Count);
    HashSet<BlockPattern>.Enumerator e = allUnseenPatterns.GetEnumerator();
    while (i > 0) {
      // these will be in a random order.
      // and we'll also walk a random number forward, so it should be pretty random.
      e.MoveNext();
      --i;
    }
    return e.Current;
  }

  public BlockPattern GetAnUnseenPattern(System.Predicate<BlockPattern> filter) {
    HashSet<BlockPattern> unseenPatterns = GetAllUnseenPatterns();
    unseenPatterns.RemoveWhere(p => !filter(p));
    int i = Random.Range(0, unseenPatterns.Count);
    HashSet<BlockPattern>.Enumerator e = unseenPatterns.GetEnumerator();
    while (i > 0) {
      e.MoveNext();
      --i;
    }
    return e.Current;
  }

  public int GetNumTotalPatterns()
  {
    int numTotalPatterns = 0;
    foreach (MemoryBank bank in memoryBanks) {
      numTotalPatterns += bank.GetNumTotalPatterns();
    }
    return numTotalPatterns;
  }

  public int GetNumSeenPatterns()
  {
    int numSeenPatterns = 0;
    foreach (MemoryBank bank in memoryBanks) {
      numSeenPatterns += bank.GetNumSeenPatterns();
    }
    return numSeenPatterns;
  }
}
