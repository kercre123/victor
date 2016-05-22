
% function test_blockMarkerExtractions()
filename = 'testTrainedCodes.png';
fullfilename = fullfile('C:\Anki\blockImages', filename);
if ~exist(fullfilename, 'file')
    fullfilename = fullfile('~/Box Sync/Cozmo SE/VisionMarkers', filename);
    
    if ~exist(fullfilename, 'file')
        error('Cannot find test image "testTrainedCodes.png".');
    end
end

image = imresize(rgb2gray(imread(fullfilename)), [240 320]);
%image(:,114:end) = 255;
%image(110:end,:) = 255;

for i_orient = 1:4
    
imageSize = size(image);

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

[quads, markerTypes, markerNames] = mexDetectFiducialMarkers(image, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_minHollowRatio, minLaplacianPeakRatio, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, quadRefinementIterations, numRefinementSamples, quadRefinementMaxCornerChange, quadRefinementMinCornerChange, returnInvalidMarkers);

for i = 1:length(quads)
    quads{i} = quads{i} + 1;
end

order = [1 2 4 3 1];

namedFigure(sprintf('test_blockMarkerExtractions %d', i_orient)); %: mexDetectFiducialMarkers()');
subplot 121
hold off;
imshow(image); 
title('mexDetectFiducialMarkers()')
hold on;
for i = 1:length(quads)
    plot(quads{i}(order,1),quads{i}(order,2), 'r');
    plot(quads{i}([1 3],1),quads{i}([1 3],2), 'g', 'LineWidth', 2);
    text(...
        mean(quads{i}(:,1)), mean(quads{i}(:,2)),...
        markerNames{i}, ...
        'HorizontalAlignment', 'center', 'BackgroundColor', [.7 .9 .7], ...
        'Interpreter', 'none');
end

markers = simpleDetector(image, 'returnInvalid', true);

%namedFigure('test_blockMarkerExtractions: Matlab simpleDetector()');
subplot 122
hold off;
imshow(image);
title('Matlab simpleDetector()')
hold on;
for i = 1:length(markers)
    if markers{i}.isValid
        name = strrep(markers{i}.name, 'MARKER_', '');
        plot(markers{i}.corners(order,1),markers{i}.corners(order,2), 'r');
        plot(markers{i}.corners([1 3],1),markers{i}.corners([1 3],2), 'g', 'LineWidth', 2);
        text(...
            mean(markers{i}.corners(:,1)), mean(markers{i}.corners(:,2)),...
            name,...
            'HorizontalAlignment', 'center', 'BackgroundColor', [.7 .9 .7], ...
            'Interpreter', 'none');
    else
        plot(markers{i}.corners(order,1),markers{i}.corners(order,2), 'b', 'LineWidth', 2);
    end
end

image = imrotate(image, 90);

end

