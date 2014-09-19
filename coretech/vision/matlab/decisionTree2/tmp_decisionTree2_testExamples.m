
function tmp_decisionTree2_testExamples()
    
    rerunTraining = false;
    
    if rerunTraining
        classesList = decisionTree2_createClassesList();
        [labelNames100, labels100, featureValues100, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'numPerturbations', 100);
        [tree100, minimalTree100, cTree100, trainingFailures100, testOnTrain_numCorrect100, testOnTrain_numTotal100] = decisionTree2_trainC(labelNames100, labels100, featureValues100, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);
        [labelNames100B, labels100B, featureValues100B, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'numPerturbations', 100);
        [tree100B, minimalTree100B, cTree100B, trainingFailures100B, testOnTrain_numCorrect100B, testOnTrain_numTotal100B] = decisionTree2_trainC(labelNames100B, labels100B, featureValues100B, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);
        [numCorrectSwitch100, numTotalSwitch100] = decisionTree2_testOnTrainingData(cTree100, featureValues100B, labels100B, labelNames100B, probeLocationsXGrid, probeLocationsYGrid, 'verbose', true);
        [numCorrectSwitch100B, numTotalSwitch100B] = decisionTree2_testOnTrainingData(cTree100B, featureValues100, labels100, labelNames100, probeLocationsXGrid, probeLocationsYGrid, 'verbose', true);
        save ~/tmp/labels100 labelNames100 labels100 featureValues100 probeLocationsXGrid probeLocationsYGrid labelNames100B labels100B featureValues100B tree100 minimalTree100 cTree100 trainingFailures100 testOnTrain_numCorrect100 testOnTrain_numTotal100 tree100B minimalTree100B cTree100B trainingFailures100B testOnTrain_numCorrect100B testOnTrain_numTotal100B
        save ~/tmp/labels100_results numCorrectSwitch100 numTotalSwitch100 numCorrectSwitch100B numTotalSwitch100B
        clear
        
        classesList = decisionTree2_createClassesList();
        [labelNames300, labels300, featureValues300, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'numPerturbations', 300);
        [tree300, minimalTree300, cTree300, trainingFailures300, testOnTrain_numCorrect300, testOnTrain_numTotal300] = decisionTree2_trainC(labelNames300, labels300, featureValues300, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);
        [labelNames300B, labels300B, featureValues300B, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'numPerturbations', 300);
        [tree300B, minimalTree300B, cTree300B, trainingFailures300B, testOnTrain_numCorrect300B, testOnTrain_numTotal300B] = decisionTree2_trainC(labelNames300B, labels300B, featureValues300B, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);
        [numCorrectSwitch300, numTotalSwitch300] = decisionTree2_testOnTrainingData(cTree300, featureValues300B, labels300B, labelNames300B, probeLocationsXGrid, probeLocationsYGrid, 'verbose', true);
        [numCorrectSwitch300B, numTotalSwitch300B] = decisionTree2_testOnTrainingData(cTree300B, featureValues300, labels300, labelNames300, probeLocationsXGrid, probeLocationsYGrid, 'verbose', true);
        save ~/tmp/labels300 labelNames300 labels300 featureValues300 probeLocationsXGrid probeLocationsYGrid labelNames300B labels300B featureValues300B tree300 minimalTree300 cTree300 trainingFailures300 testOnTrain_numCorrect300 testOnTrain_numTotal300 tree300B minimalTree300B cTree300B trainingFailures300B testOnTrain_numCorrect300B testOnTrain_numTotal300B
        save ~/tmp/labels300_results numCorrectSwitch300 numTotalSwitch300 numCorrectSwitch300B numTotalSwitch300B
        clear
        
        classesList = decisionTree2_createClassesList();
        [labelNames600, labels600, featureValues600, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'numPerturbations', 600);
        [tree600, minimalTree600, cTree600, trainingFailures600, testOnTrain_numCorrect600, testOnTrain_numTotal600] = decisionTree2_trainC(labelNames600, labels600, featureValues600, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);
        [labelNames600B, labels600B, featureValues600B, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'numPerturbations', 600);
        [tree600B, minimalTree600B, cTree600B, trainingFailures600B, testOnTrain_numCorrect600B, testOnTrain_numTotal600B] = decisionTree2_trainC(labelNames600B, labels600B, featureValues600B, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);
        [numCorrectSwitch600, numTotalSwitch600] = decisionTree2_testOnTrainingData(cTree600, featureValues600B, labels600B, labelNames600B, probeLocationsXGrid, probeLocationsYGrid, 'verbose', true);
        [numCorrectSwitch600B, numTotalSwitch600B] = decisionTree2_testOnTrainingData(cTree600B, featureValues600, labels600, labelNames600, probeLocationsXGrid, probeLocationsYGrid, 'verbose', true);
        save ~/tmp/labels600 labelNames600 labels600 featureValues600 probeLocationsXGrid probeLocationsYGrid labelNames600B labels600B featureValues600B tree600 minimalTree600 cTree600 trainingFailures600 testOnTrain_numCorrect600 testOnTrain_numTotal600 tree600B minimalTree600B cTree600B trainingFailures600B testOnTrain_numCorrect600B testOnTrain_numTotal600B
        save ~/tmp/labels600_results numCorrectSwitch600 numTotalSwitch600 numCorrectSwitch600B numTotalSwitch600B
        clear
        
        classesList = decisionTree2_createClassesList();
        [labelNames1000, labels1000, featureValues1000, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'numPerturbations', 1000);
        [tree1000, minimalTree1000, cTree1000, trainingFailures1000, testOnTrain_numCorrect1000, testOnTrain_numTotal1000] = decisionTree2_trainC(labelNames1000, labels1000, featureValues1000, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);
        [labelNames1000B, labels1000B, featureValues1000B, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'numPerturbations', 1000);
        [tree1000B, minimalTree1000B, cTree1000B, trainingFailures1000B, testOnTrain_numCorrect1000B, testOnTrain_numTotal1000B] = decisionTree2_trainC(labelNames1000B, labels1000B, featureValues1000B, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);
        [numCorrectSwitch1000, numTotalSwitch1000] = decisionTree2_testOnTrainingData(cTree1000, featureValues1000B, labels1000B, labelNames1000B, probeLocationsXGrid, probeLocationsYGrid, 'verbose', true);
        [numCorrectSwitch1000B, numTotalSwitch1000B] = decisionTree2_testOnTrainingData(cTree1000B, featureValues1000, labels1000, labelNames1000, probeLocationsXGrid, probeLocationsYGrid, 'verbose', true);
        save ~/tmp/labels1000 labelNames1000 labels1000 featureValues1000 probeLocationsXGrid probeLocationsYGrid labelNames1000B labels1000B featureValues1000B tree1000 minimalTree1000 cTree1000 trainingFailures1000 testOnTrain_numCorrect1000 testOnTrain_numTotal1000 tree1000B minimalTree1000B cTree1000B trainingFailures1000B testOnTrain_numCorrect1000B testOnTrain_numTotal1000B
        save ~/tmp/labels1000_results numCorrectSwitch1000 numTotalSwitch1000 numCorrectSwitch1000B numTotalSwitch1000B
        clear
        
        classesList = decisionTree2_createClassesList();
        [labelNames2000, labels2000, featureValues2000, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'numPerturbations', 2000);
        save ~/tmp/labels2000 labelNames2000 labels2000 featureValues2000 probeLocationsXGrid probeLocationsYGrid
        [tree2000, minimalTree2000, cTree2000, trainingFailures2000, testOnTrain_numCorrect2000, testOnTrain_numTotal2000] = decisionTree2_trainC(labelNames2000, labels2000, featureValues2000, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);
        save ~/tmp/labels2000 labelNames2000 labels2000 featureValues2000 probeLocationsXGrid probeLocationsYGrid tree2000 minimalTree2000 cTree2000 trainingFailures2000 testOnTrain_numCorrect2000 testOnTrain_numTotal2000
        clear
        
        classesList = decisionTree2_createClassesList();
        [labelNames2000B, labels2000B, featureValues2000B, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'numPerturbations', 2000);
        save ~/tmp/labels2000B labelNames2000B labels2000B featureValues2000B probeLocationsXGrid probeLocationsYGrid
        [tree2000B, minimalTree2000B, cTree2000B, trainingFailures2000B, testOnTrain_numCorrect2000B, testOnTrain_numTotal2000B] = decisionTree2_trainC(labelNames2000B, labels2000B, featureValues2000B, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);
        save ~/tmp/labels2000B labelNames2000B labels2000B featureValues2000B probeLocationsXGrid probeLocationsYGrid tree2000B minimalTree2000B cTree2000B trainingFailures2000B testOnTrain_numCorrect2000B testOnTrain_numTotal2000B
        clear
        
        load ~/tmp/labels2000 cTree2000
        load ~/tmp/labels2000B
        [numCorrectSwitch2000, numTotalSwitch2000] = decisionTree2_testOnTrainingData(cTree2000, featureValues2000B, labels2000B, labelNames2000B, probeLocationsXGrid, probeLocationsYGrid, 'verbose', true);
        save ~/tmp/labels2000_results numCorrectSwitch2000 numTotalSwitch2000
        clear
        
        load ~/tmp/labels2000
        load ~/tmp/labels2000B cTree2000B
        load ~/tmp/labels2000_results
        [numCorrectSwitch2000B, numTotalSwitch2000B] = decisionTree2_testOnTrainingData(cTree2000B, featureValues2000, labels2000, labelNames2000, probeLocationsXGrid, probeLocationsYGrid, 'verbose', true);
        save ~/tmp/labels2000_results numCorrectSwitch2000 numTotalSwitch2000 numCorrectSwitch2000B numTotalSwitch2000B
        clear
        
        classesList = decisionTree2_createClassesList();
        [labelNames3000, labels3000, featureValues3000, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'numPerturbations', 3000);
        save ~/tmp/labels3000 labelNames3000 labels3000 featureValues3000 probeLocationsXGrid probeLocationsYGrid
        [tree3000, minimalTree3000, cTree3000, trainingFailures3000, testOnTrain_numCorrect3000, testOnTrain_numTotal3000] = decisionTree2_trainC(labelNames3000, labels3000, featureValues3000, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);
        save ~/tmp/labels3000 labelNames3000 labels3000 featureValues3000 probeLocationsXGrid probeLocationsYGrid tree3000 minimalTree3000 cTree3000 trainingFailures3000 testOnTrain_numCorrect3000 testOnTrain_numTotal3000
        clear
        
        classesList = decisionTree2_createClassesList();
        [labelNames3000B, labels3000B, featureValues3000B, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'numPerturbations', 3000);
        save ~/tmp/labels3000B labelNames3000B labels3000B featureValues3000B probeLocationsXGrid probeLocationsYGrid
        [tree3000B, minimalTree3000B, cTree3000B, trainingFailures3000B, testOnTrain_numCorrect3000B, testOnTrain_numTotal3000B] = decisionTree2_trainC(labelNames3000B, labels3000B, featureValues3000B, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);
        save ~/tmp/labels3000B labelNames3000B labels3000B featureValues3000B probeLocationsXGrid probeLocationsYGrid tree3000B minimalTree3000B cTree3000B trainingFailures3000B testOnTrain_numCorrect3000B testOnTrain_numTotal3000B
        clear
        
        load ~/tmp/labels3000 cTree3000
        load ~/tmp/labels3000B
        [numCorrectSwitch3000, numTotalSwitch3000] = decisionTree2_testOnTrainingData(cTree3000, featureValues3000B, labels3000B, labelNames3000B, probeLocationsXGrid, probeLocationsYGrid, 'verbose', true);
        save ~/tmp/labels3000_results numCorrectSwitch3000 numTotalSwitch3000
        clear
        
        load ~/tmp/labels3000
        load ~/tmp/labels3000B cTree3000B
        load ~/tmp/labels3000_results
        [numCorrectSwitch3000B, numTotalSwitch3000B] = decisionTree2_testOnTrainingData(cTree3000B, featureValues3000, labels3000, labelNames3000, probeLocationsXGrid, probeLocationsYGrid, 'verbose', true);
        save ~/tmp/labels3000_results numCorrectSwitch3000 numTotalSwitch3000 numCorrectSwitch3000B numTotalSwitch3000B
        clear
    end
    
    treeSizes = zeros(12, 1);
    trainPercentCorrects = zeros(12, 1);
    testPercentCorrects = zeros(12, 1);
    
    baseFilename = '~/tmp/labels';
    filenames = {'100', '100', '300', '300', '600', '600', '1000', '1000', '2000', '2000B', '3000', '3000B'};
    suffixes = {'100', '100B', '300', '300B', '600', '600B', '1000', '1000B', '2000', '2000B', '3000', '3000B'};
    
    for iType = 1:length(filenames)
        [treeSizes(iType), trainPercentCorrects(iType), testPercentCorrects(iType)] = getResults([baseFilename, filenames{iType}], suffixes{iType});
    end
    
    indexes = repmat([100,300,600,1000,2000,3000], [2,1]); 
    indexes = indexes(:);
    
    figure(1); 
    scatter(indexes, testPercentCorrects);
    title('Percent of test set correct');
    
    figure(2);
    scatter(indexes, treeSizes);
    title('Tree size')
    
    keyboard
    
end % tmp_testExamples()

function [treeSize, trainPercentCorrect, testPercentCorrect] = getResults(filename, suffix)
    load(filename,...
        ['cTree', suffix],...
        ['testOnTrain_numCorrect', suffix],...
        ['testOnTrain_numTotal', suffix]);
    
    load([filename, '_results'],...
        ['numCorrectSwitch', suffix],...
        ['numTotalSwitch', suffix]);
    
    eval(['treeSize = length(cTree', suffix, '.depths);']);
    eval(['trainPercentCorrect = double(sum(testOnTrain_numCorrect', suffix, ')) / double(sum(testOnTrain_numTotal', suffix, '));']);
    eval(['testPercentCorrect = double(sum(numCorrectSwitch', suffix, ')) / double(sum(numTotalSwitch', suffix, '));']);
end
