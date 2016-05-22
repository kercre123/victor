
from anki import *
from sklearn import tree
import time
import numpy as np
import pickle
import pdb

labelNames = ['0_000', '0_090', '1_000', '1_090', '1_180', '1_270', '2_000', '2_090', '2_180', '2_270', '3_000', '3_090', '3_180', '3_270', '4_000', '4_090', '4_180', '4_270', '5_000', '5_090', '5_180', '5_270', '6_000', '6_090', '6_180', '6_270', '7_000', '7_090', '7_180', '7_270', '8_000', '8_090', 'A_000', 'A_090', 'A_180', 'A_270', 'B_000', 'B_090', 'B_180', 'B_270', 'C_000', 'C_090', 'C_180', 'C_270', 'D_000', 'D_090', 'D_180', 'D_270', 'E_000', 'E_090', 'E_180', 'E_270', 'F_000', 'F_090', 'F_180', 'F_270', 'G_000', 'G_090', 'G_180', 'G_270', 'H_000', 'H_090', 'J_000', 'J_090', 'J_180', 'J_270', 'K_000', 'K_090', 'K_180', 'K_270', 'L_000', 'L_090', 'L_180', 'L_270', 'M_000', 'M_090', 'M_180', 'M_270', 'N_000', 'N_090', 'P_000', 'P_090', 'P_180', 'P_270', 'Q_000', 'Q_090', 'Q_180', 'Q_270', 'R_000', 'R_090', 'R_180', 'R_270', 'T_000', 'T_090', 'T_180', 'T_270', 'X_000', 'Y_000', 'Y_090', 'Y_180', 'Y_270', 'angryFace_000', 'angryFace_090', 'angryFace_180', 'angryFace_270', 'ankiLogo_000', 'ankiLogo_090', 'ankiLogo_180', 'ankiLogo_270', 'arrow_000', 'arrow_090', 'arrow_180', 'arrow_270', 'bangBangBang_000', 'bangBangBang_090', 'bangBangBang_180', 'bangBangBang_270', 'bridgeMoonLeft_000', 'bridgeMoonLeft_090', 'bridgeMoonLeft_180', 'bridgeMoonLeft_270', 'bridgeMoonMiddle_000', 'bridgeMoonMiddle_090', 'bridgeMoonMiddle_180', 'bridgeMoonMiddle_270', 'bridgeMoonRight_000', 'bridgeMoonRight_090', 'bridgeMoonRight_180', 'bridgeMoonRight_270', 'bridgeSunLeft_000', 'bridgeSunLeft_090', 'bridgeSunLeft_180', 'bridgeSunLeft_270', 'bridgeSunMiddle_000', 'bridgeSunMiddle_090', 'bridgeSunMiddle_180', 'bridgeSunMiddle_270', 'bridgeSunRight_000', 'bridgeSunRight_090', 'bridgeSunRight_180', 'bridgeSunRight_270', 'bullseye2', 'circularArrow_000', 'circularArrow_090', 'circularArrow_180', 'circularArrow_270', 'fire_000', 'fire_090', 'fire_180', 'fire_270', 'platformEast_000', 'platformEast_090', 'platformEast_180', 'platformEast_270', 'platformNorth_000', 'platformNorth_090', 'platformNorth_180', 'platformNorth_270', 'platformSouth_000', 'platformSouth_090', 'platformSouth_180', 'platformSouth_270', 'platformWest_000', 'platformWest_090', 'platformWest_180', 'platformWest_270', 'questionMark_000', 'questionMark_090', 'questionMark_180', 'questionMark_270', 'rampBack_000', 'rampBack_090', 'rampBack_180', 'rampBack_270', 'rampFront_000', 'rampFront_090', 'rampFront_180', 'rampFront_270', 'rampLeft_000', 'rampLeft_090', 'rampLeft_180', 'rampLeft_270', 'rampRight_000', 'rampRight_090', 'rampRight_180', 'rampRight_270', 'squarePlusCorners', 'star5_000', 'star5_090', 'star5_180', 'star5_270', 'stopWithHand_000', 'stopWithHand_090', 'stopWithHand_180', 'stopWithHand_270', 'dice1', 'dice2_000', 'dice2_090', 'dice3_000', 'dice3_090', 'dice4', 'dice5', 'dice6_000', 'dice6_090', 'ankiLogoWithBits001_000', 'ankiLogoWithBits001_090', 'ankiLogoWithBits001_180', 'ankiLogoWithBits001_270', 'ankiLogoWithBits002_000', 'ankiLogoWithBits002_090', 'ankiLogoWithBits002_180', 'ankiLogoWithBits002_270', 'ankiLogoWithBits003_000', 'ankiLogoWithBits003_090', 'ankiLogoWithBits003_180', 'ankiLogoWithBits003_270', 'ankiLogoWithBits004_000', 'ankiLogoWithBits004_090', 'ankiLogoWithBits004_180', 'ankiLogoWithBits004_270', 'ankiLogoWithBits005_000', 'ankiLogoWithBits005_090', 'ankiLogoWithBits005_180', 'ankiLogoWithBits005_270', 'ankiLogoWithBits006_000', 'ankiLogoWithBits006_090', 'ankiLogoWithBits006_180', 'ankiLogoWithBits006_270', 'ankiLogoWithBits007_000', 'ankiLogoWithBits007_090', 'ankiLogoWithBits007_180', 'ankiLogoWithBits007_270', 'ankiLogoWithBits008_000', 'ankiLogoWithBits008_090', 'ankiLogoWithBits008_180', 'ankiLogoWithBits008_270', 'ankiLogoWithBits009_000', 'ankiLogoWithBits009_090', 'ankiLogoWithBits009_180', 'ankiLogoWithBits009_270', 'ankiLogoWithBits010_000', 'ankiLogoWithBits010_090', 'ankiLogoWithBits010_180', 'ankiLogoWithBits010_270', 'ankiLogoWithBits011_000', 'ankiLogoWithBits011_090', 'ankiLogoWithBits011_180', 'ankiLogoWithBits011_270', 'ankiLogoWithBits012_000', 'ankiLogoWithBits012_090', 'ankiLogoWithBits012_180', 'ankiLogoWithBits012_270', 'ankiLogoWithBits013_000', 'ankiLogoWithBits013_090', 'ankiLogoWithBits013_180', 'ankiLogoWithBits013_270', 'ankiLogoWithBits014_000', 'ankiLogoWithBits014_090', 'ankiLogoWithBits014_180', 'ankiLogoWithBits014_270', 'ankiLogoWithBits015_000', 'ankiLogoWithBits015_090', 'ankiLogoWithBits015_180', 'ankiLogoWithBits015_270', 'ankiLogoWithBits016_000', 'ankiLogoWithBits016_090', 'ankiLogoWithBits016_180', 'ankiLogoWithBits016_270', 'ankiLogoWithBits017_000', 'ankiLogoWithBits017_090', 'ankiLogoWithBits017_180', 'ankiLogoWithBits017_270', 'ankiLogoWithBits018_000', 'ankiLogoWithBits018_090', 'ankiLogoWithBits018_180', 'ankiLogoWithBits018_270', 'ankiLogoWithBits019_000', 'ankiLogoWithBits019_090', 'ankiLogoWithBits019_180', 'ankiLogoWithBits019_270', 'ankiLogoWithBits020_000', 'ankiLogoWithBits020_090', 'ankiLogoWithBits020_180', 'ankiLogoWithBits020_270', 'ankiLogoWithBits021_000', 'ankiLogoWithBits021_090', 'ankiLogoWithBits021_180', 'ankiLogoWithBits021_270', 'ankiLogoWithBits022_000', 'ankiLogoWithBits022_090', 'ankiLogoWithBits022_180', 'ankiLogoWithBits022_270', 'ankiLogoWithBits023_000', 'ankiLogoWithBits023_090', 'ankiLogoWithBits023_180', 'ankiLogoWithBits023_270', 'ankiLogoWithBits024_000', 'ankiLogoWithBits024_090', 'ankiLogoWithBits024_180', 'ankiLogoWithBits024_270', 'ankiLogoWithBits025_000', 'ankiLogoWithBits025_090', 'ankiLogoWithBits025_180', 'ankiLogoWithBits025_270', 'ankiLogoWithBits026_000', 'ankiLogoWithBits026_090', 'ankiLogoWithBits026_180', 'ankiLogoWithBits026_270', 'ankiLogoWithBits027_000', 'ankiLogoWithBits027_090', 'ankiLogoWithBits027_180', 'ankiLogoWithBits027_270', 'ankiLogoWithBits028_000', 'ankiLogoWithBits028_090', 'ankiLogoWithBits028_180', 'ankiLogoWithBits028_270', 'ankiLogoWithBits029_000', 'ankiLogoWithBits029_090', 'ankiLogoWithBits029_180', 'ankiLogoWithBits029_270', 'ankiLogoWithBits030_000', 'ankiLogoWithBits030_090', 'ankiLogoWithBits030_180', 'ankiLogoWithBits030_270', 'ankiLogoWithBits031_000', 'ankiLogoWithBits031_090', 'ankiLogoWithBits031_180', 'ankiLogoWithBits031_270', 'ankiLogoWithBits032_000', 'ankiLogoWithBits032_090', 'ankiLogoWithBits032_180', 'ankiLogoWithBits032_270', 'ankiLogoWithBits033_000', 'ankiLogoWithBits033_090', 'ankiLogoWithBits033_180', 'ankiLogoWithBits033_270', 'ankiLogoWithBits034_000', 'ankiLogoWithBits034_090', 'ankiLogoWithBits034_180', 'ankiLogoWithBits034_270', 'ankiLogoWithBits035_000', 'ankiLogoWithBits035_090', 'ankiLogoWithBits035_180', 'ankiLogoWithBits035_270', 'ankiLogoWithBits036_000', 'ankiLogoWithBits036_090', 'ankiLogoWithBits036_180', 'ankiLogoWithBits036_270', 'ankiLogoWithBits037_000', 'ankiLogoWithBits037_090', 'ankiLogoWithBits037_180', 'ankiLogoWithBits037_270', 'ankiLogoWithBits038_000', 'ankiLogoWithBits038_090', 'ankiLogoWithBits038_180', 'ankiLogoWithBits038_270', 'ankiLogoWithBits039_000', 'ankiLogoWithBits039_090', 'ankiLogoWithBits039_180', 'ankiLogoWithBits039_270', 'ankiLogoWithBits040_000', 'ankiLogoWithBits040_090', 'ankiLogoWithBits040_180', 'ankiLogoWithBits040_270', 'ankiLogoWithBits041_000', 'ankiLogoWithBits041_090', 'ankiLogoWithBits041_180', 'ankiLogoWithBits041_270', 'ankiLogoWithBits042_000', 'ankiLogoWithBits042_090', 'ankiLogoWithBits042_180', 'ankiLogoWithBits042_270', 'ankiLogoWithBits043_000', 'ankiLogoWithBits043_090', 'ankiLogoWithBits043_180', 'ankiLogoWithBits043_270', 'ankiLogoWithBits044_000', 'ankiLogoWithBits044_090', 'ankiLogoWithBits044_180', 'ankiLogoWithBits044_270', 'ankiLogoWithBits045_000', 'ankiLogoWithBits045_090', 'ankiLogoWithBits045_180', 'ankiLogoWithBits045_270', 'ankiLogoWithBits046_000', 'ankiLogoWithBits046_090', 'ankiLogoWithBits046_180', 'ankiLogoWithBits046_270', 'ankiLogoWithBits047_000', 'ankiLogoWithBits047_090', 'ankiLogoWithBits047_180', 'ankiLogoWithBits047_270', 'ankiLogoWithBits048_000', 'ankiLogoWithBits048_090', 'ankiLogoWithBits048_180', 'ankiLogoWithBits048_270', 'ankiLogoWithBits049_000', 'ankiLogoWithBits049_090', 'ankiLogoWithBits049_180', 'ankiLogoWithBits049_270', 'ankiLogoWithBits050_000', 'ankiLogoWithBits050_090', 'ankiLogoWithBits050_180', 'ankiLogoWithBits050_270', 'ankiLogoWithBits051_000', 'ankiLogoWithBits051_090', 'ankiLogoWithBits051_180', 'ankiLogoWithBits051_270', 'ankiLogoWithBits052_000', 'ankiLogoWithBits052_090', 'ankiLogoWithBits052_180', 'ankiLogoWithBits052_270', 'ankiLogoWithBits053_000', 'ankiLogoWithBits053_090', 'ankiLogoWithBits053_180', 'ankiLogoWithBits053_270', 'ankiLogoWithBits054_000', 'ankiLogoWithBits054_090', 'ankiLogoWithBits054_180', 'ankiLogoWithBits054_270', 'ankiLogoWithBits055_000', 'ankiLogoWithBits055_090', 'ankiLogoWithBits055_180', 'ankiLogoWithBits055_270', 'ankiLogoWithBits056_000', 'ankiLogoWithBits056_090', 'ankiLogoWithBits056_180', 'ankiLogoWithBits056_270', 'ankiLogoWithBits057_000', 'ankiLogoWithBits057_090', 'ankiLogoWithBits057_180', 'ankiLogoWithBits057_270', 'ankiLogoWithBits058_000', 'ankiLogoWithBits058_090', 'ankiLogoWithBits058_180', 'ankiLogoWithBits058_270', 'ankiLogoWithBits059_000', 'ankiLogoWithBits059_090', 'ankiLogoWithBits059_180', 'ankiLogoWithBits059_270', 'ankiLogoWithBits060_000', 'ankiLogoWithBits060_090', 'ankiLogoWithBits060_180', 'ankiLogoWithBits060_270', 'ankiLogoWithBits061_000', 'ankiLogoWithBits061_090', 'ankiLogoWithBits061_180', 'ankiLogoWithBits061_270', 'ankiLogoWithBits062_000', 'ankiLogoWithBits062_090', 'ankiLogoWithBits062_180', 'ankiLogoWithBits062_270', 'ankiLogoWithBits063_000', 'ankiLogoWithBits063_090', 'ankiLogoWithBits063_180', 'ankiLogoWithBits063_270', 'ankiLogoWithBits064_000', 'ankiLogoWithBits064_090', 'ankiLogoWithBits064_180', 'ankiLogoWithBits064_270']

def trainTree(labelsFilename, featureValuesFilename, numFeatures):
  t0 = time.time()
  labels = loadEmbeddedArray(labelsFilename + '.array')
   
  numImages = labels.shape[1]
    
  featureValues = np.zeros((numImages, numFeatures), 'uint8')

  #numImages = 100
  
  for iFeature in range(0, numFeatures):
    curFeatureValues = loadEmbeddedArray(featureValuesFilename + '%d' % iFeature + '.array')
    featureValues[:,iFeature] = curFeatureValues
  
  t1 = time.time()

  print('Loaded in %f' % (t1-t0))

  labelsP = [0] * numImages
  featureValuesP = [[]] * numImages

  for i in range(0,numImages):
    labelsP[i] = labels[0][i]
  
  labels = None
  
  for i in range(0,numImages):   
    featureValuesP[i] = featureValues[i,:].tolist()
    
  featureValues = None  
    
  t2 = time.time()

  print('Converted in %f' % (t2-t1))

  learnedTree = tree.DecisionTreeClassifier()
  learnedTree = learnedTree.fit(featureValuesP, labelsP)

  t3 = time.time()

  print('Trained in %f' % (t3-t2))
  
  return learnedTree

def testOnTrainingData(learnedTree, labelsFilename, featureValuesFilename, numFeatures):
  t0 = time.time()
  
  labels = loadEmbeddedArray(labelsFilename + '.array')
  
  numImages = labels.shape[1]

  featureValues = np.zeros((numImages, numFeatures), 'uint8')
  
  #numImages = 100
  
  for iFeature in range(0, numFeatures):
    curFeatureValues = loadEmbeddedArray(featureValuesFilename + '%d' % iFeature + '.array')
    featureValues[:,iFeature] = curFeatureValues
    
  t1 = time.time()
  
  print('Loaded in %f' % (t1-t0))

  maxLabelId = labels.max()

  numCorrect = np.zeros((maxLabelId+1, 1))
  numTotal = np.zeros((maxLabelId+1, 1))
  
  t2 = time.time()
  
  for iImage in range(0,numImages):
    labelId = learnedTree.predict(featureValues[iImage,:])
    labelId = labelId[0]
    
    numTotal[labelId-1] += 1
        
    if labelId == labels[0,iImage]:
      numCorrect[labelId-1] += 1
      
    if iImage % 50000 == 0 or iImage == (numImages-1):
      t3 = time.time()
      print('Tested %d/%d. Current accuracy %d/%d=%f in %f seconds' % (iImage+1, numImages, numCorrect.sum(), numTotal.sum(), numCorrect.sum()/numTotal.sum(), t3-t2));
      t2 = time.time()

  result = {'numCorrect':numCorrect, 'numTotal':numTotal}
    
  return result
  
  
  
  
relearnTrees = True;  

labelsFilenamePrefix = '/Users/pbarnum/tmp/'
featureValuesFilenamePrefix = '/Users/pbarnum/tmp/pythonFeatures/'
outFilenamePrefix = '/Users/pbarnum/tmp/'

suffixes = ['100', '300', '600']
numFeatures = 900
  
if relearnTrees:  
  learnedTreesDict = {}
  
  for suffix in suffixes:
    learnedTreesDict['tree' + suffix] = trainTree(labelsFilenamePrefix + 'labels' + suffix, featureValuesFilenamePrefix + 'featureValues' + suffix + '_', numFeatures)
    learnedTreesDict['treeBinary' + suffix] = trainTree(labelsFilenamePrefix + 'labels' + suffix, featureValuesFilenamePrefix + 'featureValuesBinary' + suffix + '_', numFeatures)
    pickle.dump(learnedTreesDict, open(outFilenamePrefix + 'learnedTreesDict.pickle', 'wb'))
else:
  pickle.load(open(outFilenamePrefix + 'learnedTreesDict.pickle', 'rb'))

accuracyDict = {}

for suffix in suffixes:
  accuracyDict['train' + suffix]  = testOnTrainingData(learnedTreesDict['tree' + suffix], labelsFilenamePrefix + 'labels' + suffix,       featureValuesFilenamePrefix + 'featureValues' + suffix + '_',  numFeatures)
  accuracyDict['switch' + suffix] = testOnTrainingData(learnedTreesDict['tree' + suffix], labelsFilenamePrefix + 'labels' + suffix + 'B', featureValuesFilenamePrefix + 'featureValues' + suffix + 'B_', numFeatures)  

  accuracyDict['trainBinary' + suffix]  = testOnTrainingData(learnedTreesDict['treeBinary' + suffix], labelsFilenamePrefix + 'labels' + suffix,       featureValuesFilenamePrefix + 'featureValuesBinary' + suffix + '_',  numFeatures)
  accuracyDict['switchBinary' + suffix] = testOnTrainingData(learnedTreesDict['treeBinary' + suffix], labelsFilenamePrefix + 'labels' + suffix + 'B', featureValuesFilenamePrefix + 'featureValuesBinary' + suffix + 'B_', numFeatures)  

  pickle.dump(accuracyDict, open(outFilenamePrefix + 'accuracyDict.pickle', 'wb'))



