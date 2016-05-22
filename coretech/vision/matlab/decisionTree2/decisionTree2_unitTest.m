% function decisionTree2_unitTest()

% Run some wierd cases for the probe tree

function decisionTree2_unitTest()
    
labelNames = {'category1', 'category2', 'category3'};
probeLocationsX = ((1:2) - .5) / 2;
probeLocationsY = ((1:2) - .5) / 2;
[probeLocationsXGrid,probeLocationsYGrid] = meshgrid(probeLocationsX, probeLocationsY);
probeLocationsXGrid = probeLocationsXGrid(:);
probeLocationsYGrid = probeLocationsYGrid(:);

passed(1) = test1(labelNames, probeLocationsXGrid, probeLocationsYGrid);
passed(2) = test2(labelNames, probeLocationsXGrid, probeLocationsYGrid);
passed(3) = test3(labelNames, probeLocationsXGrid, probeLocationsYGrid);
passed(4) = test4(labelNames, probeLocationsXGrid, probeLocationsYGrid);

passed

assert(min(passed) == 1)

% keyboard

end % decisionTree2_unitTest()

function passed = testOnBoth(labelNames, labels1, probeValues1, probeLocationsXGrid, probeLocationsYGrid, ignoreMultipleLabels)
    [probeTree1m, ~, ~, testOnTrain_numCorrect1m, testOnTrain_numTotal1c] = decisionTree2_train(labelNames, labels1, probeValues1, probeLocationsXGrid, probeLocationsYGrid, false);
    [probeTree1c, ~, ~, testOnTrain_numCorrect1c, testOnTrain_numTotal1m] = decisionTree2_train(labelNames, labels1, probeValues1, probeLocationsXGrid, probeLocationsYGrid, true);

    passed = true;
    
    if ~decisionTree2_compareTwoTrees(probeTree1m, probeTree1c, false, ignoreMultipleLabels)
        passed = false;
    end
    
    if testOnTrain_numCorrect1m ~= testOnTrain_numCorrect1c
        passed = false;
    end
        
    if testOnTrain_numTotal1c ~= testOnTrain_numTotal1m 
        passed = false;
    end
end % testOnBoth()

% Easy case. All features are great
function passed = test1(labelNames, probeLocationsXGrid, probeLocationsYGrid)
    labels1 = int32([1, 1, 1, 2, 2, 2, 3, 3, 3])';
    probeValues1 = {...
        uint8([1, 1, 1, 2, 2, 2, 3, 3, 3])',...
        uint8([1, 1, 1, 2, 2, 2, 3, 3, 3])',...
        uint8([1, 1, 1, 2, 2, 2, 3, 3, 3])',...
        uint8([1, 1, 1, 2, 2, 2, 3, 3, 3])',...
      };
  
  passed = testOnBoth(labelNames, labels1, probeValues1, probeLocationsXGrid, probeLocationsYGrid, false);
end % test1()

% Two features together are sufficient.
function passed = test2(labelNames, probeLocationsXGrid, probeLocationsYGrid)
    labels1 = int32([1, 1, 1, 2, 2, 2, 3, 3, 3])';
    probeValues1 = {...
        uint8([1, 1, 1, 2, 2, 2, 2, 2, 2])',...
        uint8([2, 2, 2, 2, 2, 2, 3, 3, 3])',...
        uint8([1, 1, 1, 1, 1, 1, 1, 1, 1])',...
        uint8([1, 1, 1, 1, 1, 1, 1, 1, 1])',...
      };
  
  passed = testOnBoth(labelNames, labels1, probeValues1, probeLocationsXGrid, probeLocationsYGrid, false);
end % test2()

% Run out of probes before splitting everything
function passed = test3(labelNames, probeLocationsXGrid, probeLocationsYGrid)
    labels1 = int32([1, 1, 1, 2, 2, 2, 3, 3, 3])';
    probeValues1 = {...
        uint8([1, 1, 1, 1, 1, 2, 2, 2, 2])',...
        uint8([1, 1, 1, 1, 2, 1, 1, 1, 1])',...
        uint8([1, 1, 1, 2, 1, 1, 1, 1, 1])',...
        uint8([1, 1, 1, 1, 1, 1, 1, 1, 2])',...
      };
  
  passed = testOnBoth(labelNames, labels1, probeValues1, probeLocationsXGrid, probeLocationsYGrid, true);
end % test3()

% Cannot split, because there's no information gain
function passed = test4(labelNames, probeLocationsXGrid, probeLocationsYGrid)
    labels1 = int32([1, 1, 1, 2, 2, 2, 3, 3, 3])';
    probeValues1 = {...
        uint8([1, 1, 1, 1, 1, 1, 1, 1, 1])',...
        uint8([1, 1, 1, 1, 1, 1, 1, 1, 1])',...
        uint8([1, 1, 1, 1, 1, 1, 1, 1, 1])',...
        uint8([1, 1, 1, 1, 1, 1, 1, 1, 1])',...
      };
  
  passed = testOnBoth(labelNames, labels1, probeValues1, probeLocationsXGrid, probeLocationsYGrid, true);
end % test4()

