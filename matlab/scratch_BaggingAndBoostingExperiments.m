datasetSize = 'medium';

switch(datasetSize)
  
  case 'large'
    data = VisionMarkerTrained.ExtractProbeValues('maxNegativeExamples', 10000, 'numPerturbations', 200, 'blurSigmas', [.001 .01], 'imageSizes', [20 40 60], 'addInversions', true, 'perturbationType', 'uniform', 'perturbSigma', 1.5);
    dataTest = VisionMarkerTrained.ExtractProbeValues('maxNegativeExamples', 500, 'numPerturbations', 10, 'blurSigmas', [.003 .002], 'imageSizes', [25 45], 'addInversions', true, 'perturbationType', 'uniform', 'perturbSigma', 1.5);

  case 'medium'
    data = VisionMarkerTrained.ExtractProbeValues('maxNegativeExamples', 5000, 'numPerturbations', 9, 'blurSigmas', [.005 .015 .03], 'imageSizes', [30 45 60], 'exposures', [0.6 1 1.8], 'addInversions', true, 'perturbationType', 'shift', 'perturbSigma', 1);
    dataTest = VisionMarkerTrained.ExtractProbeValues('maxNegativeExamples', 0, 'numPerturbations', 10, 'blurSigmas', [.012 .025], 'imageSizes', [32 38], 'exposures', [0.8 1.75], 'addInversions', true, 'perturbationType', 'uniform', 'perturbSigma', 1);
    
  case 'small'
    data     = VisionMarkerTrained.ExtractProbeValues('maxNegativeExamples', 500, 'numPerturbations', 7, 'blurSigmas', [.005 .015 .03], 'imageSizes', [30 60], 'exposures', [0.8 1.5], 'addInversions', false, 'perturbationType', 'shift', 'perturbSigma', 1);
    dataTest = VisionMarkerTrained.ExtractProbeValues('maxNegativeExamples', 100, 'numPerturbations', 5, 'blurSigmas', [.01 .03], 'imageSizes', [25 45], 'exposures', [.9 1.4], 'addInversions', false, 'perturbationType', 'uniform', 'perturbSigma', 1);
    
  case 'tiny'
    data = VisionMarkerTrained.ExtractProbeValues('markerImageDir', VisionMarkerTrained.TrainingImageDir(1:2), 'maxNegativeExamples', 0, 'numPerturbations', 10, 'blurSigmas', [.01 .05], 'imageSizes', [20 30], 'addInversions', false, 'perturbationType', 'uniform', 'perturbSigma', 0.2);
    dataTest = VisionMarkerTrained.ExtractProbeValues('markerImageDir', VisionMarkerTrained.TrainingImageDir(1:2), 'maxNegativeExamples', 0, 'numPerturbations', 3, 'blurSigmas', 0.02, 'imageSizes', 25, 'addInversions', false, 'perturbationType', 'uniform', 'perturbSigma', 0.1);

  otherwise
    error('Unrecognized datasetSize = %s', datasetSize)
end


%% Choose Threshold
if false
  thresholds = [0.5 0.54 0.58 0.6];
  tryNoThreshold = false;
  
  numThresholds = length(thresholds);
  ctrees = cell(1, numThresholds+double(tryNoThreshold));
  acc    = zeros(1, numThresholds+double(tryNoThreshold));
  for iThresh = 1:numThresholds
    fprintf('Fitting single tree using %.2f as the probe threshold.\n', thresholds(iThresh));
    ctrees{iThresh} = ClassificationTree.fit(single(data.probeValues' > thresholds(iThresh)), data.labels, 'MinLeaf', 1);
    acc(iThresh) = sum(predict(ctrees{iThresh}, single(dataTest.probeValues' > thresholds(iThresh)))' == dataTest.labels)/length(dataTest.labels);
  end
  
  if tryNoThreshold
    fprintf('Fitting single tree using no threshold.\n');
    ctrees{end} = ClassificationTree.fit(single(data.probeValues'), data.labels, 'MinLeaf', 1);
    acc(end) = sum(predict(ctrees{iThresh}, single(dataTest.probeValues'))' == dataTest.labels)/length(dataTest.labels);
  end
  
  h_fig = figure;
  if tryNoThreshold
    bar(acc)
    set(gca, 'YLim', [0.5*min(acc) 1], 'XTickLabel', [cellfun(@num2str, num2cell(thresholds), 'UniformOutput', false) 'None'])
  else
    bar(thresholds, acc);
    set(gca, 'YLim', [0.5*min(acc) 1], 'XTick', thresholds)
  end
  
  xlabel('Threshold')
  ylabel('Test Set Accuracy')
  title('Single Classification Tree')
  saveas(h_fig, sprintf('~/Documents/Anki/Ensemble Learning Experiments/thresholdChoice_%s.fig', datasetSize));
end

%% Reduce memory:
data.probeValues = single(data.probeValues'>.5);
dataTest.probeValues = single(dataTest.probeValues'>.5);
if isfield(data, 'gradMagValues')
  data = rmfield(data, 'gradMagValues');
end
if isfield(dataTest, 'gradMagValues')
  dataTest = rmfield(dataTest, 'gradMagValues');
end

%% Add Neighborhood Features
if false
  steps = [1 2];
  index = reshape(1:size(data.probeValues,1), 32, 32);
  newProbeValues = cell(1, length(steps)+1);
  newProbeValues{end} = data.probeValues > .5;
  
  for iStep = 1:length(steps)
    step = steps(iStep);
    up    = image_up(index, step);
    down  = image_down(index, step);
    left  = image_left(index, step);
    right = image_right(index, step);
    
    %newProbeValues{iStep} = [abs(data.probeValues - data.probeValues(up,:)); ...
    %  abs(data.probeValues - data.probeValues(down,:)); ...
    %  abs(data.probeValues - data.probeValues(left,:)); ...
    %  abs(data.probeValues - data.probeValues(right,:))];
    
    newProbeValues{iStep} = [newProbeValues{end} == newProbeValues{end}(up,:); ...
      newProbeValues{end} == newProbeValues{end}(down,:); ...
      newProbeValues{end} == newProbeValues{end}(left,:); ...
      newProbeValues{end} == newProbeValues{end}(right,:)];
    
  end
  
  newProbeValues = vertcat(newProbeValues{:});
end


%% Single Tree
% compare split criteria
criteria = {'gdi', 'deviance', 'twoing'};

ctrees = cell(1, length(criteria));

acc = zeros(1, length(criteria));
for iCrit = 1:length(criteria)
  t = tic;
  fprintf('Fitting single tree using "%s" as the split criterion...', criteria{iCrit});
  ctrees{iCrit} = ClassificationTree.fit(data.probeValues,data.labels, 'MinLeaf', 1, 'SplitCriterion', criteria{iCrit});
  acc(iCrit) = 1 - loss(ctrees{iCrit}, dataTest.probeValues, dataTest.labels); % sum(predict(ctrees{iCrit}, dataTest.probeValues)' == dataTest.labels)/length(dataTest.labels);
  fprintf('Done. (%.1f seconds)\n', toc(t));
end

h_fig = figure;
bar(acc)
set(gca, 'XLim', [0.5 length(criteria)+0.5], 'YLim', [0.5*min(acc) 1], 'XTickLabel', criteria)
xlabel('SplitCriterion')
ylabel('Test Set Accuracy')
title('Single Classification Tree')
saveas(h_fig, sprintf('~/Documents/Anki/Ensemble Learning Experiments/splitCriterion_%s.fig', datasetSize));

%% Choose best single tree

[~,bestSplitCriterion] = max(acc);
ctree = ctrees{bestSplitCriterion};



%% Pruning
try
  maxPruneFraction = 0.25;
  maxPruneLevel = ceil(maxPruneFraction*max(ctree.PruneList));
  testPruneLevels = unique(round(linspace(1, maxPruneLevel, 15)));
  acc = zeros(1,length(testPruneLevels));
  numNodes = zeros(1,length(testPruneLevels));
  for iLevel = 1:length(testPruneLevels)
    fprintf('iLevel = %d\n', testPruneLevels(iLevel));
    ctreePruned = ctree;
    ctreePruned.Impl = prune(ctree.Impl, 'Level', testPruneLevels(iLevel));
    acc(iLevel) = 1- loss(ctreePruned, dataTest.probeValues, dataTest.labels); %sum(ctreePruned.predict(dataTest.probeValues)' == dataTest.labels) / length(dataTest.labels);
    numNodes(iLevel) = ctreePruned.NumNodes;
  end
  
  h_fig = figure('Name', 'Pruning');
  subplot 211
  plot(testPruneLevels, acc, 'b');
  hold on
  plot(testPruneLevels, numNodes/ctree.NumNodes, 'r')
  xlabel('Pruning Level')
  ylabel('Accuracy / Fraction of Full Size')
  legend('Accuracy', 'Fraction of Original Nodes')
  title('Pruning: Accuracy / Size Tradeoff')
  grid on
  
  subplot 212
  bar(testPruneLevels, numNodes)
  xlabel('Pruning Level')
  ylabel('Number of Nodes')
  
  saveas(h_fig, sprintf('~/Documents/Anki/Ensemble Learning Experiments/pruning_%s.fig', datasetSize));
  
  clear ctreePruned
  
catch E
  warning('Pruning experiment failed: %s', E.message);
end


%% Ensemble
ensembleMethods = {'Bag'}; %{'AdaBoostM2', 'Bag'};

numTreesList = [1 2 4 6 8 10];
minLeaf = [1 2 8]; %[2 4 8 16 32 64];
pruneFractions = [.1 .2];
colors = jet(length(minLeaf));
markerTypes = 'oxs+*d';

assert(length(pruneFractions) <= length(markerTypes));
maxTrees = numTreesList(end);

for iMethod = 1:length(ensembleMethods)
  ensembleMethod = ensembleMethods{iMethod};
  
  h_fig = figure('Name', ensembleMethod);
  clf(h_fig)
  
  legendStrs = cell(length(pruneFractions)+1,length(minLeaf));
  accuracy = zeros(length(minLeaf), length(numTreesList), 1+length(pruneFractions));
  numNodes = zeros(length(minLeaf), maxTrees, 1+length(pruneFractions));
  
  for iLeaf = 1:length(minLeaf)
    fprintf('Training with minLeaf = %d\n', minLeaf(iLeaf));
    
    % Shallow Trees
    tTree = templateTree('MinLeaf', minLeaf(iLeaf), 'SplitCriterion', criteria{bestSplitCriterion}, 'Prune', 'on', 'PruneCriterion', 'error', 'MergeLeaves', 'on');
    
    % % Stumps:
    % tTree = templateTree('MinParent', size(data.probeValues,2), 'SplitCriterion', criteria{bestSplitCriterion});
    
    % Train the full ensemble
    ensembleCTrees = fitensemble(data.probeValues, data.labels, ensembleMethod, maxTrees, tTree, 'Type', 'Classification', 'NPrint', round(.1*maxTrees));
    
    % Evaluate it for different numbers of trees
    for iTree = 1:length(numTreesList)
      numTrees = numTreesList(iTree);
      
      accuracy(iLeaf,iTree,1) = 1 - loss(ensembleCTrees, dataTest.probeValues, dataTest.labels, 'learners', 1:numTrees); % sum(predict(ensembleCTrees, dataTest.probeValues, 'learners', 1:numTrees)' == dataTest.labels)/length(dataTest.labels);
    end
    
    % Sum up the number of nodes in each tree. Note that this is a
    % separate loop because the iTree loop above only loops over the
    % entries in numTreesList, not _all_ trained trees.
    for iTree = 1:length(ensembleCTrees.Trained)
      numNodes(iLeaf,iTree,1) = ensembleCTrees.Trained{iTree}.NumNodes;
    end
        
    for iPrune = 1:length(pruneFractions)
      fprintf('\tPruning fraction = %.2f\n', pruneFractions(iPrune));
      prunedEnsemble = ensembleCTrees;
      jTree = 1;
      for iTree = 1:length(numTreesList)
        numTrees = numTreesList(iTree);
        
        while jTree <= numTrees
          pruneLevel = ceil(pruneFractions(iPrune)*max(prunedEnsemble.Trained{jTree}.PruneList));
          prunedEnsemble.Impl.Trained{jTree}.Impl = prune(prunedEnsemble.Impl.Trained{jTree}.Impl, 'Level', pruneLevel);
          fprintf('\t\tPruned tree %d from %d nodes to %d nodes\n', jTree, ensembleCTrees.Trained{jTree}.NumNodes, prunedEnsemble.Trained{jTree}.NumNodes);
          jTree = jTree + 1;
        end
        
        accuracy(iLeaf,iTree,1+iPrune) = 1 - loss(prunedEnsemble, dataTest.probeValues, dataTest.labels, 'learners', 1:numTrees); %sum(predict(prunedEnsemble, dataTest.probeValues, 'learners', 1:numTrees)' == dataTest.labels)/length(dataTest.labels);
      end
      
      % Sum up the number of nodes in each tree. Note that this is a
      % separate loop because the iTree loop above only loops over the
      % entries in numTreesList, not _all_ trained trees.
      for iTree = 1:length(prunedEnsemble.Trained)
        numNodes(iLeaf,iTree,1+iPrune) = prunedEnsemble.Trained{iTree}.NumNodes;
      end
      
    end % FOR each pruneFraction
    
    
    for iPlot = 1:size(accuracy,3)
      subplot 211
      plot(numTreesList, accuracy(iLeaf,:,iPlot), [markerTypes(iPlot) '-'], 'Color', colors(iLeaf,:));
      hold on
      
      subplot 212
      plot(1:maxTrees, cumsum(numNodes(iLeaf,:,iPlot)), [markerTypes(iPlot) '-'], 'Color', colors(iLeaf,:));
      hold on
      
      drawnow
      
      pruneFraction = 0;
      if iPlot > 1
        pruneFraction = pruneFractions(iPlot-1);
      end
      legendStrs{iPlot,iLeaf} = sprintf('MinLeaf = %d (Pruning=%.2f)', minLeaf(iLeaf), pruneFraction);
    end % FOR each plot
    
    saveas(h_fig, sprintf('~/Documents/Anki/Ensemble Learning Experiments/%s_%s.fig', ensembleMethod, datasetSize));
    
  end % FOR each minLeaf value
  
  subplot 211
  plot([0 maxTrees], sum(predict(ctree, dataTest.probeValues)' == dataTest.labels)/length(dataTest.labels)*[1 1], 'k--');
  legend(legendStrs{:}, 'Single Tree');
  title(sprintf('Ensemble Accuracy vs. Num Trees (%s)', ensembleMethod))
  xlabel('Num Trees');
  ylabel('Test Set Accuracy')
  set(gca, 'YLim', [0.75 1]);
  
  subplot 212
  plot([0 maxTrees], ctree.NumNodes*[1 1], 'k--');
  title('Total Nodes vs. Num Trees')
  xlabel('Num Trees')
  ylabel('Total Num Nodes')
  
  saveas(h_fig, sprintf('~/Documents/Anki/Ensemble Learning Experiments/%s_%s.fig', ensembleMethod, datasetSize));
  
end % FOR each ensemble method

%% Examing variation of single class

index = find(data.labels == 30);

for i = 1:length(index)
  subplot 121
  temp = reshape(data.probeValues(:,index(i)), 32, 32) > mean(temp(:));
  imagesc(temp, [0 1])
  axis image
  
  subplot 122
  hist(temp(:), 32)
  
  pause 
end

%% Testing on Real Data

  % Strip rotation from labelNames and add 'MARKER_'
  labelNamesUnrotated = cellfun(@(name)['MARKER_' upper(strrep(strrep(strrep(strrep(name, '_000', ''), '_090', ''), '_270', ''), '_180', ''))], data.labelNames, 'UniformOutput', false);

  DEBUG_DISPLAY = true;
  
  t = tic;  fprintf('Predicting with single tree...');
  numExamples = size(realData.probeValues,1);
  predictedLabels = zeros(numExamples,1);
  
  tree = VisionMarkerTrained.ProbeTree;
  if ~iscell(tree)
    tree = {tree};
  end
  numTrees = length(tree);
  
  for iExample = 1:numExamples
    labelIDs = zeros(1, numTrees);
    for iTree = 1:numTrees
      
      [~, temp] = TestTree(tree{iTree}, ...
        realData.probeValues(iExample,:)', [], 128, VisionMarkerTrained.ProbePattern);
      if ~isscalar(temp)
        labelIDs(iTree) = mode(temp);
      else
        labelIDs(iTree) = temp;
      end
    end
    predictedLabels(iExample) = mode(labelIDs);
    if sum(labelIDs == predictedLabels(iExample)) < .5*numTrees
      predictedLabels(iExample) = length(labelNamesUnrotated);
    end
    if DEBUG_DISPLAY && ~strcmp(labelNamesUnrotated{predictedLabels(iExample)}, realData.markerTypes{iExample}) && ~strcmp(labelNamesUnrotated{predictedLabels(iExample)}, 'MARKER_INVALID')
      imagesc(reshape(realData.probeValues(iExample,:), 32, 32)), axis image
      title({sprintf('Predicted: %s', labelNamesUnrotated{predictedLabels(iExample)}), ...
        sprintf('Truth: %s', realData.markerTypes{iExample})}, ...
        'Interp', 'none')
      pause
    end
  end
  
  %predictedLabels = predict(ctree, single(realData.probeValues));
  correct   = strcmp(labelNamesUnrotated(predictedLabels), realData.markerTypes(1:numExamples));
  invalids  = strcmp(labelNamesUnrotated(predictedLabels), 'MARKER_INVALID');
  
  % "True Positives" are the ones we labeled correctly. 
  truePosRate  = sum(correct) / length(correct);
  
  % "False Positives" are those that we labeled incorrectly -- without 
  % calling them "invalid". So we reported the wrong thing. These are 
  % *real bad*.
  falsePosRate = sum(~correct & ~invalids) / length(correct);
  
  % "False Negative" are those we did not identify but that we at least
  % labelled as "invalid", meaning we just missed them and didn't see
  % the _wrong_ thing. These are *less bad*.
  falseNegRate = sum(~correct &  invalids) / length(correct);
  
  fprintf('Done (%.1f seconds)\n', toc(t));
  
  %%
  
  numTreesList = [1 2 4 8 10];
  acc = zeros(1, length(numTreesList));
  acc2 = zeros(1, length(numTreesList));
  accPruned = zeros(1, length(numTreesList));
  accPruned2 = zeros(1, length(numTreesList));
  
  for iTreeList = 1:length(numTreesList)
    iTree = numTreesList(iTreeList);
    fprintf('Predicting with %d of %d trees in the ensemble.\n', iTree, ensembleCTrees.NumTrained);
    predictedLabels = predict(ensembleCTrees, single(realData.probeValues), 'learners', 1:iTree);
    
    predictedLabels2 = predict(ensembleCTrees, single(realData.probeValues2), 'learners', 1:iTree);
    
    predictedLabelsPruned  = predict(prunedEnsemble, single(realData.probeValues), 'learners', 1:iTree);
    predictedLabelsPruned2 = predict(prunedEnsemble, single(realData.probeValues2), 'learners', 1:iTree);
    
    acc(iTreeList) = sum(strcmp(labelNamesUnrotated(predictedLabels), realData.markerTypes)) / length(realData.markerTypes);    
    acc2(iTreeList) = sum(strcmp(labelNamesUnrotated(predictedLabels2), realData.markerTypes)) / length(realData.markerTypes);    
    
    accPruned(iTreeList) = sum(strcmp(labelNamesUnrotated(predictedLabelsPruned), realData.markerTypes)) / length(realData.markerTypes);    
    accPruned2(iTreeList) = sum(strcmp(labelNamesUnrotated(predictedLabelsPruned2), realData.markerTypes)) / length(realData.markerTypes);    
  end
  
  hold off
  plot([0 ensembleCTrees.NumTrained], accSingle*[1 1], 'k--');
  hold on
  plot(numTreesList, acc, 'r')
  plot(numTreesList, acc2, 'g')
  plot(numTreesList, accPruned, 'b')
  plot(numTreesList, accPruned2, 'c')
  
  legend('single', 'bagged', 'bagged2', 'bagged+pruned', 'bagged2+pruned')

