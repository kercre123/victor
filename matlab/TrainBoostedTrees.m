function trees = TrainBoostedTrees(data, varargin)

numBoostingIterations = 5;
useProbability = true;
sampleFraction = 1;
maxDepth = 4;

parseVarargin(varargin{:});

if sampleFraction < 1
    sampleIndex = rand(1,size(data.probeValues,2)) < sampleFraction;
    data.probeValues = data.probeValues(:,sampleIndex);
    data.gradMagValues = data.gradMagValues(:,sampleIndex);
    data.labels = data.labels(sampleIndex);
    data.numImages = sum(sampleIndex);
end

weights = ones(1,size(data.probeValues,2))/size(data.probeValues,2);


pBar = ProgressBar('Boosting');
pBar.showTimingInfo = true;
pBarCleanup = onCleanup(@()delete(pBar));
pBar.set_message(sprintf('Training %d boosted trees.', numBoostingIterations));
pBar.set_increment(1/numBoostingIterations);
pBar.set(0);

alphas = cell(1, numBoostingIterations);
for iBoost = 1:numBoostingIterations
    trees(iBoost) = VisionMarkerTrained.TrainProbeTree('trainingState', data, 'maxDepth', maxDepth, ...
        'baggingSampleFraction', 1, 'redBlackVerifyDepth', 0, 'saveTree', false, 'testTree', false, 'weights', weights);
    
    [weights, alphas{iBoost}] = UpdateBoostingWeights(trees(iBoost), data.probeValues, data.labels, weights, useProbability);
    
    pBar.increment();
end

% Put alphas inside tree
for iBoost = 1:numBoostingIterations
    trees(iBoost).alpha = alphas{iBoost};
end

end