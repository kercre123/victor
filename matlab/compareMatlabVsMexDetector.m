function compareMatlabVsMexDetector(varargin)
% Helper to run detectAndDisplay with CameraCapture.  Passes extra args to CameraCapture.


CamCapArgs = parseVarargin(varargin{:});

CameraCapture('resolution', [320 240], 'processFcn', @detectionHelper, ...
    'doContinuousProcessing', true, CamCapArgs{:});

end


function detectionHelper(img, h_axes, h_img)

axis(h_axes, 'on');
set(h_axes, 'LineWidth', 5, 'Box', 'on', 'XTick', [], 'YTick', [], 'XLim', [1 2*size(img,2)]);
set(h_img, 'CData', [img img]);

img = rgb2gray(img);
imageSize = size(img);

if isempty(img) || any(size(img)==0)
    warning('Empty image passed in!');
    return
end

%% Matlab detection
matlabDetections = simpleDetector(img, 'quadRefinementIterations', 0);

%% Mex Detection
scaleImage_thresholdMultiplier = .75;
scaleImage_numPyramidLevels = 3;
component1d_minComponentWidth = 0;
component1d_maxSkipDistance = 0;
minSideLength = round(0.03*max(imageSize(1),imageSize(2)));
maxSideLength = round(0.97*min(imageSize(1),imageSize(2)));
component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
component_sparseMultiplyThreshold = 1000.0;
component_solidMultiplyThreshold = 2.0;
component_minHollowRatio = 1.0;
quads_minQuadArea = 100 / 4;
quads_quadSymmetryThreshold = 2.0;
quads_minDistanceFromImageEdge = 2;
decode_minContrastRatio = 1.25;
returnInvalidMarkers = 0;
quadRefinementIterations = 0;

[quads, ~, markerNames] = mexDetectFiducialMarkers(img, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_minHollowRatio, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, quadRefinementIterations, returnInvalidMarkers);

for i = 1:length(quads)
    quads{i} = quads{i} + 1;
end

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
    matlabDetections{i}.Draw('Tag', 'VisionMarkers', 'Parent', h_axes);
end
order = [1 2 4 3 1];
ncols = size(img,2);
for i = 1:numMexDetections
    name = strrep(markerNames{i}, 'MARKER_', '');
    plot(quads{i}(order,1)+ncols,quads{i}(order,2), 'r', 'LineWidth', 3, 'Tag', 'VisionMarkers');
    plot(quads{i}([1 3],1)+ncols,quads{i}([1 3],2), 'g', 'LineWidth', 3, 'Tag', 'VisionMarkers');
    text(...
        mean(quads{i}(:,1))+ncols, mean(quads{i}(:,2)),...
        name, ...
        'HorizontalAlignment', 'center', 'BackgroundColor', [.7 .9 .7], ...
        'Interpreter', 'none', 'Tag', 'VisionMarkers');
end

drawnow

%fprintf('Rest of detectAndDisplay took %.2f seconds\n', toc(t));


end
