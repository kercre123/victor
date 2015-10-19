using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class PatternMemory {
  
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

  public void AddSeen(BlockPattern pattern) {
    // Seems like this would be a good point to return (Seen/ New / Not_A_Pattern)
    DAS.Info("PatternMemory.AddSeen", "Seen pattern " + pattern.GetHashCode());
    for (int i = 0; i < memoryBanks.Count; ++i) {
      if (memoryBanks[i].Contains(pattern)) {
        if (memoryBanks[i].TryAddSeen(pattern)) {
          DAS.Info("PatternMemory.Add", "Pattern Added for bank: " + memoryBanks[i].name);
        }
        else {
          DAS.Info("PatternMemory.Add", "Pattern already exists in bank: " + memoryBanks[i].name);
        }
        DAS.Info("PatternMemory.Add", "There are now " + memoryBanks[i].GetSeenPatterns().Count + " patterns in that bank");
        return;
      }
    }

    DAS.Info("PatternMemory.Add.InvalidPattern", "pattern does not match any registered in PatternMemory");
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

}
