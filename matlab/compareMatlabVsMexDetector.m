function compareMatlabVsMexDetector(varargin)
% Helper to run detectAndDisplay with CameraCapture.  Passes extra args to CameraCapture.

ShowCornerSpread = false;

CamCapArgs = parseVarargin(varargin{:});

if ShowCornerSpread
    h_spreadFig = namedFigure('CornerPosition');
    clf(h_spreadFig);
    h_spreadAxes = axes('Parent', h_spreadFig);
    hold(h_spreadAxes, 'on');
end

CameraCapture('resolution', [320 240], 'processFcn', @detectionHelper, ...
    'doContinuousProcessing', true, CamCapArgs{:});


function detectionHelper(img, h_axes, h_img)

axis(h_axes, 'on');
set(h_axes, 'LineWidth', 5, 'Box', 'on', 'XTick', [], 'YTick', [], 'XLim', [1 2*size(img,2)]);
set(h_img, 'CData', [img img]);

if size(img,3)>1
    img = rgb2gray(img);
end

imageSize = size(img);

if isempty(img) || any(size(img)==0)
    warning('Empty image passed in!');
    return
end


%% Mex Detection
 useIntegralImageFiltering = true;
 scaleImage_thresholdMultiplier = 1.0;
 %scaleImage_thresholdMultiplier = 0.75;
 scaleImage_numPyramidLevels = 3;
 component1d_minComponentWidth = 0;
 component1d_maxSkipDistance = 0;
 minSideLength = round(0.01*max(imageSize(1),imageSize(2)));
 maxSideLength = round(0.97*min(imageSize(1),imageSize(2)));
 component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
 component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
 component_sparseMultiplyThreshold = 1000.0;
 component_solidMultiplyThreshold = 2.0;
 component_minHollowRatio = 1.0;
 minLaplacianPeakRatio = 5;
 quads_minQuadArea = 100 / 4;
 quads_quadSymmetryThreshold = 2.0;
 quads_minDistanceFromImageEdge = 2;
 decode_minContrastRatio = 1.25;
 quadRefinementIterations = 25;
 numRefinementSamples = 100;
 quadRefinementMaxCornerChange = 5;
 quadRefinementMinCornerChange = .005;
 returnInvalidMarkers = 0;
 [quads, ~, markerNames, ~] = mexDetectFiducialMarkers(img, useIntegralImageFiltering, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_minHollowRatio, minLaplacianPeakRatio, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, quadRefinementIterations, numRefinementSamples, quadRefinementMaxCornerChange, quadRefinementMinCornerChange, returnInvalidMarkers);

for i = 1:length(quads)
    quads{i} = quads{i} + 1;
end

%% Matlab detection
matlabDetections = simpleDetector(img, 'quadRefinementIterations', quadRefinementIterations);


%% Display

%t = tic;
numMatlabDetections = length(matlabDetections);
numMexDetections = length(quads);

h_detections = findobj(h_axes, 'Tag', 'VisionMarkers');
if ~isempty(h_detections)
    delete(h_detections)
end

hold(h_axes, 'on')

for i = 1:numMatlabDetections
    matlabDetections{i}.Draw('Tag', 'VisionMarkers', ...
        'FontSize', 14, 'Parent', h_axes);
    
    if ShowCornerSpread
        corners = matlabDetections{i}.corners;
        plot(corners(:,1), corners(:,2), 'rx', 'Parent', h_spreadAxes);
    end
end
order = [1 2 4 3 1];
ncols = size(img,2);
for i = 1:numMexDetections
    name = strrep(markerNames{i}, 'MARKER_', '');
    plot(quads{i}(order,1)+ncols,quads{i}(order,2), 'r', 'LineWidth', 3, 'Tag', 'VisionMarkers', 'Parent', h_axes);
    plot(quads{i}([1 3],1)+ncols,quads{i}([1 3],2), 'g', 'LineWidth', 3, 'Tag', 'VisionMarkers', 'Parent', h_axes);
    text(...
        mean(quads{i}(:,1))+ncols, mean(quads{i}(:,2)),...
        name, ...
        'HorizontalAlignment', 'center', 'BackgroundColor', [.7 .9 .7], ...
        'Interpreter', 'none', 'Tag', 'VisionMarkers', 'Parent', h_axes);
    
    if ShowCornerSpread
        plot(quads{i}(:,1), quads{i}(:,2), 'bo', 'Parent', h_spreadAxes);
    end
end

drawnow

%fprintf('Rest of detectAndDisplay took %.2f seconds\n', toc(t));


end

end
