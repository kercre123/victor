datasetSize = 'medium';

switch(datasetSize)
  
  case 'large'
    data = VisionMarkerTrained.ExtractProbeValues('maxNegativeExamples', 10000, 'numPerturbations', 200, 'blurSigmas', [.001 .01], 'imageSizes', [20 40 60], 'addInversions', true, 'perturbationType', 'uniform', 'perturbSigma', 1.5);
    dataTest = VisionMarkerTrained.ExtractProbeValues('maxNegativeExamples', 500, 'numPerturbations', 10, 'blurSigmas', [.003 .002], 'imageSizes', [25 45], 'addInversions', true, 'perturbationType', 'uniform', 'perturbSigma', 1.5);

  case 'medium'
    data = VisionMarkerTrained.ExtractProbeValues('maxNegativeExamples', 5000, 'numPerturbations', 50, 'blurSigmas', [.001 .01], 'imageSizes', [20 40 60], 'addInversions', true, 'perturbationType', 'uniform', 'perturbSigma', 1);
    dataTest = VisionMarkerTrained.ExtractProbeValues('maxNegativeExamples', 100, 'numPerturbations', 10, 'blurSigmas', .003, 'imageSizes', [25 45], 'addInversions', true, 'perturbationType', 'uniform', 'perturbSigma', 1);
    
  case 'small'
    data = VisionMarkerTrained.ExtractProbeValues('maxNegativeExamples', 500, 'numPerturbations', 20, 'blurSigmas', [.001 .01], 'imageSizes', [20 60], 'addInversions', false, 'perturbationType', 'uniform', 'perturbSigma', 1);
    dataTest = VisionMarkerTrained.ExtractProbeValues('maxNegativeExamples', 100, 'numPerturbations', 5, 'blurSigmas', .003, 'imageSizes', [25 45], 'addInversions', false, 'perturbationType', 'uniform', 'perturbSigma', 1);

  otherwise
    error('Unrecognized datasetSize = %s', datasetSize)
end

% Reduce memory:
data.probeValues = single(data.probeValues'>.5);
dataTest.probeValues = single(dataTest.probeValues'>.5);
data = rmfield(data, 'gradMagValues');
dataTest = rmfield(dataTest, 'gradMagValues');

%% Single Tree
% compare split criteria
criteria = {'gdi', 'deviance'}; %, 'twoing'};

ctrees = cell(1, length(criteria));

acc = zeros(1, length(criteria));
for iCrit = 1:length(criteria)
  ctrees{iCrit} = ClassificationTree.fit(data.probeValues,data.labels, 'MinLeaf', 1, 'SplitCriterion', criteria{iCrit});
  acc(iCrit) = sum(predict(ctrees{iCrit}, dataTest.probeValues)' == dataTest.labels)/length(dataTest.labels);
end

h_fig = figure;
bar(acc)
set(gca, 'YLim', [0.9 1], 'XTickLabel', criteria)
xlabel('SplitCriterion')
ylabel('Test Set Accuracy')
title('Single Classification Tree')
saveas(h_fig, sprintf('~/Documents/Anki/Ensemble Learning Experiments/splitCriterion_%s.fig', datasetSize));

%% Choose best single tree

[~,bestSplitCriterion] = max(acc);
ctree = ctrees{bestSplitCriterion};

%% Pruning
try
  maxPruneFraction = 0.5;
  maxPruneLevel = ceil(maxPruneFraction*max(ctree.PruneList));
  testPruneLevels = unique(round(linspace(1, maxPruneLevel, 15)));
  acc = zeros(1,length(testPruneLevels));
  numNodes = zeros(1,length(testPruneLevels));
  for iLevel = 1:length(testPruneLevels)
    fprintf('iLevel = %d\n', testPruneLevels(iLevel));
    ctreePruned = ctree;
    ctreePruned.Impl = prune(ctree.Impl, 'Level', testPruneLevels(iLevel));
    acc(iLevel) = sum(ctreePruned.predict(dataTest.probeValues)' == dataTest.labels) / length(dataTest.labels);
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
ensembleMethods = {'AdaBoostM2', 'Bag'};

numTreesList = [1 2 4 8 12 16];
minLeaf = [1 8 16]; %[2 4 8 16 32 64];
pruneFractions = [.1 .25 .5];
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
      
      accuracy(iLeaf,iTree,1) = sum(predict(ensembleCTrees, dataTest.probeValues, 'learners', 1:numTrees)' == dataTest.labels)/length(dataTest.labels);
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
        
        accuracy(iLeaf,iTree,1+iPrune) = sum(predict(prunedEnsemble, dataTest.probeValues, 'learners', 1:numTrees)' == dataTest.labels)/length(dataTest.labels);
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


