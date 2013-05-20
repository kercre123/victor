function [features, labels] = trainFiducialDetector(varargin)

%% Parameters
features = {};
labels = {};
winSize = 12;
numIntensityHistBins = 64;
positiveExampleDir = '/Users/andrew/Code/blockIdentification/positivePatches';
negativeExampleDir = '/Users/andrew/Code/blockIdentification/negativePatches';
useBilinearBinning = false;
blurSigma = 1;
trainFraction = 0.75;
%fprThreshold = 20; % percent
tprThreshold = 97.5; % percent

parseVarargin(varargin{:});

%% Initialize
C = onCleanup(@()multiWaitbar('closeAll'));

assert(mod(winSize,2)==0, 'Expecting even winSize');
centerIndex = false(winSize);
mid = winSize/2;
centerIndex(mid:(mid+1),mid:(mid+1)) = true;

edgeIndex = false(winSize);
%edgeIndex(1,[1 winSize/2 winSize])        = true;
%edgeIndex(winSize/2, [1 winSize])         = true;
%edgeIndex(winSize, [1 winSize/2 winSize]) = true;
edgeIndex([1 end],:) = true;
edgeIndex(:,[1 end]) = true;

%positiveHist = eps*ones(numIntensityHistBins);
%negativeHist = eps*ones(numIntensityHistBins);


%% Loop over examples
if isempty(features)

    posExamples = getfnames(positiveExampleDir, '*.png', 'useFullPath', true);
    negExamples = getfnames(negativeExampleDir, '*.png', 'useFullPath', true);
    
    fprintf('Training using %d positive and %d negative examples.\n', ...
        length(posExamples), length(negExamples));
    
    examples = [posExamples; negExamples];
    exampleLabels = [ones(length(posExamples),1); -ones(length(negExamples),1)];
    
    
    numExamples = length(examples);
    multiWaitbar('Example', 0);
    multiWaitbar('Example', 'CanCancel', 'on');
    
    features = cell(numExamples,1);
    labels   = cell(numExamples,1);
    
    [examples, index] = sample_array(examples);
    exampleLabels = exampleLabels(index);
    
    for i_example = 1:numExamples
        
        img = imread(examples{i_example});
        img = mean(img, 3);
        if any(size(img)~=winSize)
            img = imresize(img, [winSize winSize], 'lanczos3');
        end
        img = uint8(img);
        
        % Get center intensity (average of 4 pixels at center)
        centerIntensity = mean(img(centerIndex));
        
        % Get edge intensities:
        edgeIntensities = img(edgeIndex);
        
        features{i_example} = [edgeIntensities(:) ...
            centerIntensity*ones(length(edgeIntensities),1)];
        
        labels{i_example} = exampleLabels(i_example)*ones(length(edgeIntensities),1);
        
        cancelled = multiWaitbar('Example', 'Increment', 1/numExamples);
        
        if cancelled
            disp('Training cancelled.');
            break;
        end
        
    end % FOR each example
    
else
    numExamples = length(features);
    assert(iscell(labels) && length(labels) == numExamples, ...
        'Labels should be a cell array of the same length as features.');
end
%% Split into train/test
numTrain = round(trainFraction*numExamples);
trainIndex = false(numExamples,1);
trainIndex(1:numTrain) = true;

featuresTrain = vertcat(features{trainIndex});
labelsTrain   = vertcat(labels{trainIndex});

% Get histogram bin:
featuresScaled = (numIntensityHistBins-1)*(double(featuresTrain)/255);


if useBilinearBinning
    % TODO: update
    
    dC = featuresScaled - (centerBin-1); %#ok<UNRCH>
    dE = edgeScaled - (edgeBins-1);
    
    bin = floor(featuresScaled) + 1;
    rightIndex = sub2ind([numIntensityHistBins numIntensityHistBins], ...
        edgeBins, min(numIntensityHistBins, (centerBin+1)*ones(size(edgeBins))));
    downIndex = sub2ind([numIntensityHistBins numIntensityHistBins], ...
        min(numIntensityHistBins, edgeBins+1), centerBin*ones(size(edgeBins)));
    downRightIndex = sub2ind([numIntensityHistBins numIntensityHistBins], ...
        min(numIntensityHistBins, edgeBins+1), ...
        min(numIntensityHistBins, (centerBin+1)*ones(size(edgeBins))));
    
    % Add examples to appropriate histogram using bilinear interpolation
    if exampleLabels(i_example) > 0
        positiveHist(binIndex) = positiveHist(binIndex) + (1-dC)*(1-dE);
        positiveHist(rightIndex) = positiveHist(rightIndex) + dC*(1-dE);
        positiveHist(downIndex) = positiveHist(downIndex) + (1-dC)*dE;
        positiveHist(downRightIndex) = positiveHist(downRightIndex) + dC*dE;
        
    else
        negativeHist(binIndex) = negativeHist(binIndex) + (1-dC)*(1-dE);
        negativeHist(rightIndex) = negativeHist(rightIndex) + dC*(1-dE);
        negativeHist(downIndex) = negativeHist(downIndex) + (1-dC)*dE;
        negativeHist(downRightIndex) = negativeHist(downRightIndex) + dC*dE;
    end
    
else
    % Simple binning   
    bin = round(featuresScaled) + 1;
    posLabels = labelsTrain > 0;
    negLabels = labelsTrain < 0;
    positiveHist = full(sparse(bin(posLabels,1), bin(posLabels,2), 1, ...
        numIntensityHistBins, numIntensityHistBins));
    
    negativeHist = full(sparse(bin(negLabels,1), bin(negLabels,2), 1, ...
            numIntensityHistBins, numIntensityHistBins));
        
    
end % IF useBilinearBinning
    
g = gaussian_kernel(blurSigma);
positiveHist = separable_filter(positiveHist, g);
negativeHist = separable_filter(negativeHist, g);

positiveHist = max(eps, positiveHist);
negativeHist = max(eps, negativeHist);

positiveHist = positiveHist/sum(positiveHist(:));
negativeHist = negativeHist/sum(negativeHist(:));

save histograms.mat positiveHist negativeHist


%% Use histograms to build classifier

namedFigure('Histograms');

subplot 221, imagesc(positiveHist), axis image xy
title('Positive Class Histogram')
xlabel('Center Intensity');
ylabel('Edge Intensity');

subplot 222, imagesc(negativeHist), axis image xy
title('Negative Class Histogram')
xlabel('Center Intensity');
ylabel('Edge Intensity');

subplot 223, imagesc(positiveHist > negativeHist), axis image xy
title('Decision Boundary')
xlabel('Center Intensity');
ylabel('Edge Intensity');

colormap(gray)

%% Create ROC

% Get histogram bin:
labelsTest = cellfun(@(x)x(1), labels(~trainIndex));
scoresTest = cellfun(@(F)trainScore(F,positiveHist,negativeHist), ...
    features(~trainIndex));

subplot 224
[tpr, fpr, thresh] = ROC(scoresTest, labelsTest, 'linespec', 'b', 'log_scale', false);

% fpr will be decreasing
%index = find(fpr < fprThreshold, 1);
index = find(tpr < tprThreshold, 1);
threshold = thresh(index);
%fprintf('True-positive rate at %d%% false positives = %.1f%%, threshold = %.3f\n', ...
%    fprThreshold, tpr(index), threshold);
fprintf('False-positive rate at %.1f%% true positives is %.1f%%, using threshold = %.3f\n', ...
   tprThreshold, fpr(index), threshold);

%% Examine errors:

testIndex = find(~trainIndex);

% False positives:
namedFigure('False Positives');
clf
FP_index = find(scoresTest > threshold & labelsTest==-1);
FP_index = sample_array(FP_index, 64);
for i = 1:min(64, length(FP_index))
    subplot(8,8,i)
    imshow(examples{testIndex(FP_index(i))})
    title(scoresTest(FP_index(i)))
end
fix_subplots(8,8, .3)


% False negatives:
namedFigure('False Negatives');
clf
FN_index = find(scoresTest < threshold & labelsTest==1);
FN_index = sample_array(FN_index, 64);
for i = 1:min(64, length(FN_index))
    subplot(8,8,i)
    imshow(examples{testIndex(FN_index(i))})
    title(scoresTest(FN_index(i)))
end
fix_subplots(8,8, .3)



%%
keyboard


end


function score = trainScore(features, posHist, negHist)
    %assert(all(labels==labels(1)), 'All labels for each window should be same.');
    
    numBins = size(posHist,1);
    bin = floor((numBins-1)*(double(features)/255))+1;
    index = sub2ind([numBins numBins], bin(:,1), bin(:,2));
    
    % All samples for the box must be above threshold, so just return the
    % lowest score (if it's above the threshold, all others must have been)
    score = min(log(posHist(index)) - log(negHist(index)));
    
end

