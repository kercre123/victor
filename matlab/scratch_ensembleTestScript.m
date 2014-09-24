N = 50;

testMode = 'boosted';

testOnTrainingData = false;

if testOnTrainingData
    sampleIndex = randi(size(data.probeValues,2), N);
else
   %testImages = randi(length(data.fnames), N);
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
        Corners = [.5 .5 32.5 32.5; .5 32.5 .5 32.5]';
        tform = cp2tform([VisionMarkerTrained.ProbeRegion(1) VisionMarkerTrained.ProbeRegion(1) VisionMarkerTrained.ProbeRegion(2) VisionMarkerTrained.ProbeRegion(2); VisionMarkerTrained.ProbeRegion(1) VisionMarkerTrained.ProbeRegion(2) VisionMarkerTrained.ProbeRegion(1) VisionMarkerTrained.ProbeRegion(2)]', Corners, 'projective');
        [~,fname,~] = fileparts(data.fnames{data.labels(sampleIndex(i))});
        
    else
        [testImg, ~, alpha] = imread(data.fnames{testImages(i)});
        [~,fname,~] = fileparts(data.fnames{testImages(i)});
        
        testImg = mean(im2double(testImg),3);
        testImg(alpha<.5) = 1;
        
        testImg = separable_filter(imresize(testImg, .4*rand+.5), gaussian_kernel(.5*rand + 1));
        
        Corners = VisionMarkerTrained.GetFiducialCorners(size(testImg,1), false) + .001*randn(4,2);
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
        case 'bagged'
            p = TestTreeEnsemble(baggedTrees, testImg, tform, .5, VisionMarkerTrained.ProbePattern);
            
            %     if max(p) < 1
            %     disp('here')
            %     keyboard
            %     end
            
            [p_sorted, sortIndex] = sort(p, 'descend');
            if p_sorted(1) > 1.5*p_sorted(2)
                predictedClass = baggedTrees(1).labels{sortIndex(1)};
                correct(i) = double(strcmp(predictedClass, fname));
            else
                predictedClass = 'UNKNOWN';
                correct(i) = -1;
            end
        case 'boosted'
            C = TestBoostedTrees(boostedTrees(1:150), testImg, tform, 0.5, VisionMarkerTrained.ProbePattern);
            predictedClass = boostedTrees(1).labels{C};
            correct(i) = double(strcmp(predictedClass, fname));
        otherwise
            error('Unknown testMode %s', testMode);
    end
    
    pBar.increment();
    h_axes = subplot(numPlotRows,numPlotCols,i);
    hold off
    if testOnTrainingData
        testImg = double(reshape(testImg, [32 32]));
    end
    imagesc(testImg), axis image
    hold on
    plot(Corners([1 2 4 3 1],1), Corners([1 2 4 3 1],2), 'm-o');
    title(predictedClass, 'Interpreter', 'none')
    xlabel(sprintf('p_{max} = %.2f', p_sorted(1)));
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
    
    pad = 5;
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
    N = N+ VisionMarkerTrained.GetNumTreeNodes(tree(iTree));
end
fprintf('Total ensemble nodes = %d\n', N);