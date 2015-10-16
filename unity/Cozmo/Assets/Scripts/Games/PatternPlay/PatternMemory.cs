using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class PatternMemory {
  
  private List<MemoryBank> memoryBanks = new List<MemoryBank>();
  private HashSet<BlockPattern> keyPatterns = new HashSet<BlockPattern>();

  public void Initialize() {

    // Load the pattern from the text asset
    TextAsset patternMemoryJSONStr = Resources.Load("Data/BlockPatterns", typeof(TextAsset)) as TextAsset;
    Debug.Assert(patternMemoryJSONStr != null);
    JSONObject patternMemoryObj = new JSONObject(patternMemoryJSONStr.text);
    Debug.Log(patternMemoryJSONStr);
    // Unpack the patterns
    Debug.Assert(patternMemoryObj.IsObject);
    foreach (string key in patternMemoryObj.keys) {
      MemoryBank bank = new MemoryBank(key);
      JSONObject bankObj = patternMemoryObj[key];
      Debug.Assert(bankObj.IsArray);
      foreach (JSONObject pattern in bankObj.list) {
        BlockPattern curPattern = new BlockPattern();
        curPattern.verticalStack = pattern["vertical"].b;
        curPattern.facingCozmo = pattern["facing_cozmo"].b;
        Debug.Assert(pattern["blocks"].IsArray);
        foreach (JSONObject blockData in pattern["blocks"].list) {
          Debug.Assert(blockData.IsObject);
          BlockLights curBlock = new BlockLights();
          curBlock.front = (blockData["front"].str != "");
          curBlock.back = (blockData["back"].str != "");
          curBlock.left = (blockData["left"].str != "");
          curBlock.right = (blockData["right"].str != "");
          curPattern.blocks.Add(curBlock);
        }
        bank.Add(curPattern);
        // currently these two are required at the pattern memory layer.
        bank.verticalStack = curPattern.verticalStack;
        bank.facingCozmo = curPattern.facingCozmo;
      }
      memoryBanks.Add(bank);
    }

    // adding key patterns.

  }

  public List<MemoryBank> GetMemoryBanks() {
    return memoryBanks;
  }

  public void AddSeen(BlockPattern pattern) {
    // Seems like this would be a good point to return (Seen/ New / Not_A_Pattern)
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

}
