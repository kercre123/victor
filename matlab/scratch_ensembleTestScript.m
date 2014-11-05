N = 50;

testMode = 'BoostedClassificationTree';

testOnTrainingData = false;

if testOnTrainingData
  sampleIndex = randi(size(data.probeValues,2), 1, N);
else
  testImages = randi(length(data.fnames), 1, N);
end

correct = zeros(1,N);
numPlotCols = ceil(sqrt(N));
numPlotRows = ceil(N/numPlotCols);
clf

pBar = ProgressBar('Testing');
pBar.set_increment(1/N);
pBar.showTimingInfo = true;
pBar.set(0);

for i=1:N
    
    if testOnTrainingData
        testImg = double(data.probeValues(:,sampleIndex(i)));
        G = VisionMarkerTrained.ProbeParameters.GridSize;
        Corners = [.5 .5 G+.5 G+.5; .5 G+.5 .5 G+.5]';
        tform = cp2tform([VisionMarkerTrained.ProbeRegion(1) VisionMarkerTrained.ProbeRegion(1) VisionMarkerTrained.ProbeRegion(2) VisionMarkerTrained.ProbeRegion(2); VisionMarkerTrained.ProbeRegion(1) VisionMarkerTrained.ProbeRegion(2) VisionMarkerTrained.ProbeRegion(1) VisionMarkerTrained.ProbeRegion(2)]', Corners, 'projective');
        [~,fname,~] = fileparts(data.labelNames{data.labels(sampleIndex(i))});
        
    else
        [testImg, ~, alpha] = imread(data.fnames{testImages(i)});
        [~,fname,~] = fileparts(data.fnames{testImages(i)});
        
        testImg = mean(im2double(testImg),3);
        testImg(alpha<.5) = 1;
        
        imgSize = randi([20 90],1);
        imgBlur = rand + 0.5;
        testImg = separable_filter(imresize(testImg, imgSize*[1 1], 'bilinear'), ...
          gaussian_kernel(imgBlur));
        
        Corners = VisionMarkerTrained.GetFiducialCorners(size(testImg,1), false) + 1*randn(4,2);
        tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
    end
    
    switch(testMode)
        case 'single'
            [~,labelID] = TestTree(singleTree, testImg, tform, .5, VisionMarkerTrained.ProbePattern);
            numLabels = length(singleTree(1).labels);
            labelHist = hist(labelID, 1:numLabels);
            [sortedCounts,sortedLabels] = sort(labelHist, 'descend');
            if sortedCounts(1) > 1*sortedCounts(2)
                predictedClass = singleTree.labels{sortedLabels(1)};
                correct(i) = double(strcmp(predictedClass, fname));
            else
                predictedClass = 'UNKNOWN';
                correct(i) = -1;
            end
        case {'bagged', 'boosted'}
          
          switch(testMode)
            case 'bagged'
              p = TestTreeEnsemble(baggedTrees, testImg, tform, .5, VisionMarkerTrained.ProbePattern);
            case 'boosted'
              p = TestBoostedTrees(boostedTrees(1), testImg, tform, 0.5, VisionMarkerTrained.ProbePattern);
            otherwise
              assert(false)
          end
          
          [p_sorted, sortIndex] = sort(p, 'descend');
          if p_sorted(1) > 1*p_sorted(2)
            predictedClass = data.labelNames{sortIndex(1)};
            correct(i) = double(strcmp(predictedClass, fname));
          else
            predictedClass = 'UNKNOWN';
            correct(i) = -1;
          end
          
      case {'ClassificationTree', 'BoostedClassificationTree'}
        if testOnTrainingData
          probeValues = testImg;
        else
          probeValues = VisionMarkerTrained.GetProbeValues(testImg, tform);
        end
        
        if strcmp(testMode, 'ClassificationTree')
          predictedClass = ctree.predict(single(probeValues(:)' < .5));
        else
          predictedClass = boostedCTrees.predict(single(probeValues(:)'<.5));
        end
        
        correct(i) = double(strcmp(predictedClass, fname));
        
      otherwise
            error('Unknown testMode %s', testMode);
    end
    
    pBar.increment();
    h_axes = subplot(numPlotRows,numPlotCols,i);
    hold off
    if testOnTrainingData
        testImg = double(reshape(testImg, VisionMarkerTrained.ProbeParameters.GridSize*[1 1]));
    end
    imagesc(testImg), axis image
    hold on
    plot(Corners([1 2 4 3 1],1), Corners([1 2 4 3 1],2), 'm-o');
    title(predictedClass, 'Interpreter', 'none')
    %xlabel(sprintf('p ratio = %.2f', p_sorted(1) - p_sorted(2)));
    switch(correct(i))
        case 0
            color = 'r';
        case 1
            color = 'g';
        case -1
            color = [.8 .5 0];
        otherwise
            error('WTF')
    end
    
    pad = .15 * size(testImg,1);
    set(h_axes, 'Box', 'on', 'XColor', color, 'YColor', color, 'LineWidth', 2, ...
        'XLim', [-pad size(testImg,2)+pad], 'YLim', [-pad size(testImg,1)+pad], ...
        'XTick', [], 'YTick', []);
        %'XLim', [.9*min(Corners(:,1)-.5) 1.1*max(Corners(:,1)+.5)], ...
        %'YLim', [.9*min(Corners(:,2)-.5) 1.1*max(Corners(:,2)+.5)]);
    
end

set(gcf, 'Name', sprintf('%s: %d correct', testMode, sum(correct==1)));

delete(pBar)

%% count ensemble nodes

N = 0;
for iTree = 1:length(tree)
    N = N+ VisionMarkerTrained.GetNumTreeNodes(b(iTree));
end
fprintf('Total ensemble nodes = %d\n', N);