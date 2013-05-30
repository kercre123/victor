function [edgeCenPosHist, edgeCenNegHist, svmModel] = trainFiducialDetector(varargin)

%% Parameters
useSymmetry = false;
numEdgeCenProbes = 16;
edgeCenterFeatures = {};
edgeCenterLabels = {};
winSize = 11;
numIntensityHistBins = 64;
positiveExampleDir = '/Users/andrew/Code/blockIdentificationData/positivePatches';
negativeExampleDir = '/Users/andrew/Code/blockIdentificationData/negativePatches';
useBilinearBinning = false;
blurSigma = 1;
trainFraction = 0.75;
%fprThreshold = 20; % percent
tprThreshold = 99.5; % percent

parseVarargin(varargin{:});

assert(mod(winSize,2)==1, 'Expecting odd winSize');

%% Initialize
C = onCleanup(@()multiWaitbar('closeAll'));

[edgeCenterProbes, symmetryProbes] = computeProbeLocations(...
    'blockHalfWidth', (winSize-1)/2, ...
    'numEdgeCenterProbes', numEdgeCenProbes);


%% Loop over examples
if isempty(edgeCenterFeatures)

    posExamples = getfnames(positiveExampleDir, '*.png', 'useFullPath', true);
    negExamples = getfnames(negativeExampleDir, '*.png', 'useFullPath', true);
    
    fprintf('Training using %d positive and %d negative examples.\n', ...
        length(posExamples), length(negExamples));
    
    examples = [posExamples; negExamples];
    exampleLabels = [ones(length(posExamples),1); -ones(length(negExamples),1)];
    
    numExamples = length(examples);
    multiWaitbar('Example', 0);
    multiWaitbar('Example', 'CanCancel', 'on');
    
    img = cell(numExamples,1);
    edgeCenterFeatures = cell(numExamples,1);
    edgeCenterLabels   = cell(numExamples,1);
    if useSymmetry
        symmetryFeatures   = cell(numExamples,1);
        symmetryLabels     = cell(numExamples,1);
    end
    
    [examples, index] = sample_array(examples);
    exampleLabels = exampleLabels(index);
    
    for i_example = 1:numExamples
        
        img{i_example} = imread(examples{i_example});
        img{i_example} = mean(img{i_example}, 3);
        if any(size(img{i_example})~=winSize)
            img{i_example} = imresize(img{i_example}, [winSize winSize], 'lanczos3');
        end
        img{i_example} = uint8(img{i_example});
       
        edgeCenterFeatures{i_example} = img{i_example}(edgeCenterProbes);
        
        edgeCenterLabels{i_example} = exampleLabels(i_example) * ...
            ones(size(edgeCenterFeatures{i_example},1),1);
        
        if useSymmetry
            symmetryFeatures{i_example} = img{i_example}(symmetryProbes);
            
            symmetryLabels{i_example} = exampleLabels(i_example) * ...
                ones(size(symmetryFeatures{i_example},1),1);
        end
        
        cancelled = multiWaitbar('Example', 'Increment', 1/numExamples);
        
        if cancelled
            disp('Training cancelled.');
            break;
        end
        
    end % FOR each example
    
else
    numExamples = length(edgeCenterFeatures);
    assert(iscell(edgeCenterLabels) && length(edgeCenterLabels) == numExamples, ...
        'Labels should be a cell array of the same length as features.');
    
    if useSymmetry
        assert(iscell(symmetryLabels) && length(symmetryLabels) == numExamples && ...
            iscell(symmetryFeatures) && length(symmetryFeatures) == numExamples, ...
            ['Symmetry labels/features should be cell arrays of the same ' ...
            'length as edgeCenterFeatures.']);
    end
    
end

%% Split into train/test
trainIndex = false(numExamples,1);
trainIndex(rand(numExamples,1) <= trainFraction & ...
    ~cellfun(@isempty,edgeCenterLabels)) = true;

edgeCenFeaturesTrain = double(vertcat(edgeCenterFeatures{trainIndex}))/255;
edgeCenLabelsTrain   = vertcat(edgeCenterLabels{trainIndex});

if useSymmetry
    symFeaturesTrain = double(vertcat(symmetryFeatures{trainIndex}))/255;
    symLabelsTrain = vertcat(symmetryLabels{trainIndex});
end

if useBilinearBinning
    % TODO: update
    
    dC = featuresScaled - (centerBin-1); 
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
        edgeCenPosHist(binIndex) = edgeCenPosHist(binIndex) + (1-dC)*(1-dE);
        edgeCenPosHist(rightIndex) = edgeCenPosHist(rightIndex) + dC*(1-dE);
        edgeCenPosHist(downIndex) = edgeCenPosHist(downIndex) + (1-dC)*dE;
        edgeCenPosHist(downRightIndex) = edgeCenPosHist(downRightIndex) + dC*dE;
        
    else
        edgeCenNegHist(binIndex) = edgeCenNegHist(binIndex) + (1-dC)*(1-dE);
        edgeCenNegHist(rightIndex) = edgeCenNegHist(rightIndex) + dC*(1-dE);
        edgeCenNegHist(downIndex) = edgeCenNegHist(downIndex) + (1-dC)*dE;
        edgeCenNegHist(downRightIndex) = edgeCenNegHist(downRightIndex) + dC*dE;
    end
    
else
    % Simple binning   
   
    [edgeCenPosHist, edgeCenNegHist] = createHistograms(edgeCenFeaturesTrain, ...
        edgeCenLabelsTrain, numIntensityHistBins, blurSigma);
    
    if useSymmetry
        [symPosHist, symNegHist] = createHistograms(symFeaturesTrain, ...
            symLabelsTrain, numIntensityHistBins, blurSigma);
    end
    
end % IF useBilinearBinning
    
save histograms.mat edgeCenPosHist edgeCenNegHist 

if useSymmetry
    save histograms.mat symPosHist symNegHist -append
end

%% Use histograms to build classifier


%% Create ROC

% Get histogram bin:
testIndex = find(~trainIndex & ~cellfun(@isempty, edgeCenterLabels));
numTest = length(testIndex);
labelsTest = zeros(numTest,1);
edgeCenScoresTest = zeros(numTest,1);
if useSymmetry
    symScoresTest = zeros(numTest,1);
end
for i_test = 1:numTest
    i = testIndex(i_test);
    
    labelsTest(i_test) = edgeCenterLabels{i}(1);
    edgeCenScoresTest(i_test) = trainScore(edgeCenterFeatures{i}, edgeCenPosHist, edgeCenNegHist);
    
    if useSymmetry
        assert(edgeCenterLabels{i}(1) == ...
            symmetryLabels{i}(1), ...
            'Labels should agree for edgeCenter and Symmetry.');
        
        symScoresTest(i_test) = trainScore(symmetryFeatures{i}, symPosHist, symNegHist);
    end
end

namedFigure('Test Set ROC');
hold off
[tprEdgeCen, fprEdgeCen, threshEdgeCen] = ROC(edgeCenScoresTest, labelsTest, 'linespec', 'b', 'log_scale', false);

% fpr will be decreasing
%index = find(fpr < fprThreshold, 1);
index = find(tprEdgeCen < tprThreshold, 1);
thresholdEdgeCen = threshEdgeCen(index);
%fprintf('True-positive rate at %d%% false positives = %.1f%%, threshold = %.3f\n', ...
%    fprThreshold, tpr(index), threshold);
fprintf('Edge-Center: False-positive rate at %.1f%% true positives is %.1f%%, using threshold = %.3f\n', ...
   tprThreshold, fprEdgeCen(index), thresholdEdgeCen);

if useSymmetry
    hold on %#ok<*UNRCH>
    [tprSym, fprSym, threshSym] = ROC(symScoresTest, labelsTest, 'linespec', 'r', 'log_scale', false);
    
    index = find(tprSym < tprThreshold, 1);
    thresholdSym = threshSym(index);
    fprintf('Symmetry: False-positive rate at %.1f%% true positives is %.1f%%, using threshold = %.3f\n', ...
        tprThreshold, fprSym(index), thresholdSym);
    
    cascadeClassifierTest = zeros(numTest,1);
    for i_test = 1:numTest
        i = testIndex(i_test);
        
        if trainScore(edgeCenterFeatures{i}, edgeCenPosHist, edgeCenNegHist) > thresholdEdgeCen
            result = trainScore(symmetryFeatures{i}, symPosHist, symNegHist);
        else
            result = -inf;
        end
        cascadeClassifierTest(i_test) = result;
        
    end
    
    [tprCascade, fprCascade, threshCascade] = ROC(cascadeClassifierTest, labelsTest, 'linespec', 'g', 'log_scale', false);
    
    index = find(tprCascade < tprThreshold, 1);
    thresholdCascade = threshCascade(index);
    fprintf('Cascade: False-positive rate at %.1f%% true positives is %.1f%%, using threshold = %.3f\n', ...
        tprThreshold, fprCascade(index), thresholdCascade);
    
    legend('Edge-Center', 'Symmetry', 'Cascade');
end

% recall = sum(cascadeClassifierTest > 0 & labelsTest > 0) / sum(labelsTest>0);
% precision = sum(labelsTest(cascadeClassifierTest > 0) > 0) / sum(cascadeClassifierTest > 0);
% 
% fprintf('Combined: %d%% precision and %d%% recall.\n', ...
%     round(100*precision), round(100*recall))

%% Display histograms:


namedFigure('Edge-Center Histograms');

subplot 131, imagesc(edgeCenPosHist), axis image xy
title('Positive Class Histogram')
xlabel('Center Intensity');
ylabel('Edge Intensity');

subplot 132, imagesc(edgeCenNegHist), axis image xy
title('Negative Class Histogram')
xlabel('Center Intensity');
ylabel('Edge Intensity');

subplot 133 %, imagesc(edgeCenPosHist > edgeCenNegHist), axis image xy
imagesc(log(edgeCenPosHist) - log(edgeCenNegHist) > thresholdEdgeCen ), axis image xy
title('Decision Boundary')
xlabel('Center Intensity');
ylabel('Edge Intensity');

colormap(gray)

if useSymmetry
    namedFigure('Symmetry Histograms')
    subplot 131, imagesc(symPosHist), axis image xy
    title('Positive Class Histogram')
    xlabel('Side1 Intensity');
    ylabel('Side2 Intensity');
    
    subplot 132, imagesc(symNegHist), axis image xy
    title('Negative Class Histogram')
    xlabel('Side1 Intensity');
    ylabel('Side2 Intensity');
    
    subplot 133 %, imagesc(symPosHist > symNegHist), axis image xy
    imagesc(log(symPosHist) - log(symNegHist) > thresholdSym ), axis image xy
    title('Decision Boundary')
    xlabel('Side1 Intensity');
    ylabel('Side2 Intensity');
    
    colormap(gray)
    
end

%% Examine errors:

% False positives:
namedFigure('Edge-Center False Positives');
clf
FP_index = find(edgeCenScoresTest > thresholdEdgeCen & labelsTest==-1);
FP_index = sample_array(FP_index, 64);
for i = 1:min(64, length(FP_index))
    subplot(8,8,i)
    imshow(examples{testIndex(FP_index(i))})
    title(edgeCenScoresTest(FP_index(i)))
end
fix_subplots(8,8, .3)


% False negatives:
namedFigure('Edge-Center False Negatives');
clf
FN_index = find(edgeCenScoresTest < thresholdEdgeCen & labelsTest==1);
FN_index = sample_array(FN_index, 64);
for i = 1:min(64, length(FN_index))
    subplot(8,8,i)
    imshow(examples{testIndex(FN_index(i))})
    title(edgeCenScoresTest(FN_index(i)))
end
fix_subplots(8,8, .3)

if useSymmetry
    % False positives:
    namedFigure('Symmetry False Positives');
    clf
    FP_index = find(symScoresTest > thresholdSym & labelsTest==-1);
    FP_index = sample_array(FP_index, 64);
    for i = 1:min(64, length(FP_index))
        subplot(8,8,i)
        imshow(examples{testIndex(FP_index(i))})
        title(symScoresTest(FP_index(i)))
    end
    fix_subplots(8,8, .3)
    
    
    % False negatives WITHOUT symmetry:
    namedFigure('Symmetry False Negatives');
    clf
    FN_index = find(symScoresTest < thresholdSym & labelsTest==1);
    FN_index = sample_array(FN_index, 64);
    for i = 1:min(64, length(FN_index))
        subplot(8,8,i)
        imshow(examples{testIndex(FN_index(i))})
        title(symScoresTest(FN_index(i)))
    end
    fix_subplots(8,8, .3)
    
end

%% Train/Test SVM:

imgTrain  = cellfun(@(x)x(:),img(trainIndex),'UniformOutput', false);
imgTrain  = im2double([imgTrain{:}]');
svmTrainLabels = cellfun(@(x)x(1),edgeCenterLabels(trainIndex));
svmModel = svmtrain(svmTrainLabels, imgTrain, '-t 0');

imgTest  = cellfun(@(x)x(:),img(testIndex),'UniformOutput', false);
imgTest  = im2double([imgTest{:}]');
svmTestLabels = cellfun(@(x)x(1),edgeCenterLabels(testIndex));
svmpredict(svmTestLabels, imgTest, svmModel);

save svmModel.mat svmModel



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


function [posHist, negHist] = createHistograms(features, labels, numBins, blurSigma)

bin = round((numBins-1)*features) + 1;

posLabels = labels > 0;
negLabels = labels < 0;

posHist = full(sparse(bin(posLabels,1), bin(posLabels,2), 1, ...
    numBins, numBins));

negHist = full(sparse(bin(negLabels,1), bin(negLabels,2), 1, ...
    numBins, numBins));

if blurSigma > 0
    g = gaussian_kernel(blurSigma);
    posHist = separable_filter(posHist, g);
    negHist = separable_filter(negHist, g);
end

posHist = max(eps, posHist);
negHist = max(eps, negHist);

posHist = posHist/sum(posHist(:));
negHist = negHist/sum(negHist(:));

end % createHistograms()

